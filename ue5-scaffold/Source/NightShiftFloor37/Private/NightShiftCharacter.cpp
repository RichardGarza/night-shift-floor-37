#include "NightShiftCharacter.h"
#include "GameConfig.h"
#include "RifleComponent.h"
#include "ArenaCollision.h"
#include "ArenaGameMode.h"
#include "NightShiftFloor37.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraShakeBase.h"
#include "Engine/DamageEvents.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

ANightShiftCharacter::ANightShiftCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Capsule ~1.8 m (DESIGN eye height)
	GetCapsuleComponent()->InitCapsuleSize(42.f, 90.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
	GetCharacterMovement()->JumpZVelocity = 500.f;       // 5 m/s
	GetCharacterMovement()->MaxWalkSpeed = 600.f;        // 6 m/s
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->GravityScale = 1.5306f;      // ~15 m/s²

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 250.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SocketOffset = FVector(0.f, 60.f, 60.f); // right shoulder OTS default
	CameraBoom->bDoCollisionTest = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Rifle = CreateDefaultSubobject<URifleComponent>(TEXT("Rifle"));
	ArenaCollision = CreateDefaultSubobject<UArenaCollision>(TEXT("ArenaCollision"));

	// Greybox body: engine cylinder scaled to the capsule. No collision — the capsule owns that.
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (CylinderMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CylinderMesh.Object);
	}
	if (ShapeMat.Succeeded())
	{
		BodyMesh->SetMaterial(0, ShapeMat.Object);
	}
	BodyMesh->SetRelativeScale3D(FVector(0.84f, 0.84f, 1.8f));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	BodyMesh->SetCastShadow(true);
}

void ANightShiftCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (!GameConfig)
	{
		if (const AArenaGameMode* GM = Cast<AArenaGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GameConfig = GM->GameConfig;
		}
	}
	GameConfig = UGameConfig::ResolveOrCreate(this, GameConfig);
	ApplyResolvedGameConfig();
	Health = GameConfig ? GameConfig->PlayerMaxHealth : 100.f;
	TimeSinceLastDamage = GameConfig ? GameConfig->PlayerRegenDelaySeconds : 5.f;
	LastGroundedZ = GetActorLocation().Z;
	bWasMovingOnGround = GetCharacterMovement() && GetCharacterMovement()->IsMovingOnGround();
	bRightShoulder = true;
	ApplyShoulderOffset();

	if (Rifle && GameConfig)
	{
		Rifle->InitializeFromConfig(GameConfig);
	}

	EnsureMappingContext();

	// OTS pitch range: no staring at your own feet or the sky.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->ViewPitchMin = -55.f;
			PC->PlayerCameraManager->ViewPitchMax = 45.f;
		}
		PC->SetControlRotation(GetActorRotation());
	}

	if (BodyMesh)
	{
		if (UMaterialInstanceDynamic* MID = BodyMesh->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.12f, 0.16f, 0.24f));
		}
	}

	BroadcastHealthChanged();

	UE_LOG(LogNightShift, Log, TEXT("ANightShiftCharacter ready — HP %.0f, walk %.0f cm/s"),
		Health, GetCharacterMovement()->MaxWalkSpeed);
}

void ANightShiftCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// DESIGN: clamp dt spikes (~50 ms) — prefer GameMode clamp; local guard here too
	const float MaxDt = GameConfig ? GameConfig->MaxDeltaTimeClampSeconds : 0.05f;
	if (DeltaSeconds > MaxDt)
	{
		DeltaSeconds = MaxDt;
	}

	if (!IsAlive())
	{
		return;
	}

	SnapshotGroundedZIfNeeded();
	UpdateRegen(DeltaSeconds);
	UpdateRecoilRecovery(DeltaSeconds);
	UpdateSprintSpeed();
}

void ANightShiftCharacter::SnapshotGroundedZIfNeeded()
{
	const bool bGrounded = GetCharacterMovement() && GetCharacterMovement()->IsMovingOnGround();
	if (bWasMovingOnGround && !bGrounded)
	{
		// Leaving ground — snapshot height for fall damage (DESIGN >6 m)
		LastGroundedZ = GetActorLocation().Z;
	}
	bWasMovingOnGround = bGrounded;
}

void ANightShiftCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	const UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!Move)
	{
		return;
	}

	const bool bWasGrounded =
		PrevMovementMode == MOVE_Walking || PrevMovementMode == MOVE_NavWalking;
	const bool bNowAirborne =
		Move->MovementMode == MOVE_Falling || Move->MovementMode == MOVE_Flying;

	if (bWasGrounded && bNowAirborne)
	{
		LastGroundedZ = GetActorLocation().Z;
		bWasMovingOnGround = false;
	}
	else if (Move->IsMovingOnGround())
	{
		bWasMovingOnGround = true;
	}
}

void ANightShiftCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ANightShiftCharacter::Move);
		}
		if (LookAction)
		{
			EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ANightShiftCharacter::Look);
		}
		if (JumpAction)
		{
			EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ANightShiftCharacter::TryJumpOrMantle);
			EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
		if (SprintAction)
		{
			EIC->BindAction(SprintAction, ETriggerEvent::Started, this, &ANightShiftCharacter::StartSprint);
			EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &ANightShiftCharacter::StopSprint);
		}
		if (FireAction)
		{
			EIC->BindAction(FireAction, ETriggerEvent::Started, this, &ANightShiftCharacter::StartFire);
			EIC->BindAction(FireAction, ETriggerEvent::Completed, this, &ANightShiftCharacter::StopFire);
		}
		if (ReloadAction)
		{
			EIC->BindAction(ReloadAction, ETriggerEvent::Started, this, &ANightShiftCharacter::RequestReload);
		}
		if (ShoulderSwapAction)
		{
			EIC->BindAction(ShoulderSwapAction, ETriggerEvent::Started, this, &ANightShiftCharacter::SwapShoulder);
		}
		if (PauseAction)
		{
			EIC->BindAction(PauseAction, ETriggerEvent::Started, this, &ANightShiftCharacter::RequestPause);
		}
	}
}


void ANightShiftCharacter::ApplyResolvedGameConfig()
{
	if (!GameConfig)
	{
		return;
	}
	ApplyConfigToMovement();
	ConfigureCapsuleFromConfig();
	if (Rifle)
	{
		Rifle->InitializeFromConfig(GameConfig);
	}
	if (ArenaCollision)
	{
		ArenaCollision->GameConfig = GameConfig;
	}
}

void ANightShiftCharacter::ApplyConfigToMovement()
{
	if (!GameConfig || !GetCharacterMovement())
	{
		return;
	}
	GetCharacterMovement()->MaxWalkSpeed = GameConfig->WalkSpeed;
	GetCharacterMovement()->JumpZVelocity = GameConfig->JumpZVelocity;
	GetCharacterMovement()->GravityScale = GameConfig->GravityScale;
}

void ANightShiftCharacter::ConfigureCapsuleFromConfig()
{
	if (!GameConfig)
	{
		return;
	}
	GetCapsuleComponent()->SetCapsuleSize(GameConfig->CapsuleRadiusCm, GameConfig->CapsuleHalfHeightCm);
}

void ANightShiftCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	BuildRuntimeInputDefaults();
}

void ANightShiftCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	EnsureMappingContext();
}

void ANightShiftCharacter::BuildRuntimeInputDefaults()
{
	// Editor-assigned assets (INPUT_MAPPING.md) win. Anything left null is built here so the
	// game is playable straight from C++ with the DESIGN key table.
	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type) -> UInputAction*
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = Type;
		return Action;
	};
	if (!MoveAction)         { MoveAction = MakeAction(TEXT("IA_Move_Runtime"), EInputActionValueType::Axis2D); }
	if (!LookAction)         { LookAction = MakeAction(TEXT("IA_Look_Runtime"), EInputActionValueType::Axis2D); }
	if (!JumpAction)         { JumpAction = MakeAction(TEXT("IA_Jump_Runtime"), EInputActionValueType::Boolean); }
	if (!SprintAction)       { SprintAction = MakeAction(TEXT("IA_Sprint_Runtime"), EInputActionValueType::Boolean); }
	if (!FireAction)         { FireAction = MakeAction(TEXT("IA_Fire_Runtime"), EInputActionValueType::Boolean); }
	if (!ReloadAction)       { ReloadAction = MakeAction(TEXT("IA_Reload_Runtime"), EInputActionValueType::Boolean); }
	if (!ShoulderSwapAction) { ShoulderSwapAction = MakeAction(TEXT("IA_ShoulderSwap_Runtime"), EInputActionValueType::Boolean); }
	if (!PauseAction)        { PauseAction = MakeAction(TEXT("IA_Pause_Runtime"), EInputActionValueType::Boolean); }

	if (DefaultMappingContext)
	{
		return;
	}
	UInputMappingContext* IMC = NewObject<UInputMappingContext>(this, TEXT("IMC_NightShift_Runtime"));
	auto Negate = [IMC](bool bX, bool bY) -> UInputModifier*
	{
		UInputModifierNegate* N = NewObject<UInputModifierNegate>(IMC);
		N->bX = bX;
		N->bY = bY;
		N->bZ = false;
		return N;
	};
	auto SwizzleYX = [IMC]() -> UInputModifier*
	{
		UInputModifierSwizzleAxis* S = NewObject<UInputModifierSwizzleAxis>(IMC);
		S->Order = EInputAxisSwizzle::YXZ;
		return S;
	};
	// WASD → Axis2D (X = right, Y = forward). Bool keys land on X, so W/S swizzle into Y.
	IMC->MapKey(MoveAction, EKeys::W).Modifiers.Add(SwizzleYX());
	{
		FEnhancedActionKeyMapping& M = IMC->MapKey(MoveAction, EKeys::S);
		M.Modifiers.Add(Negate(true, true));
		M.Modifiers.Add(SwizzleYX());
	}
	IMC->MapKey(MoveAction, EKeys::A).Modifiers.Add(Negate(true, true));
	IMC->MapKey(MoveAction, EKeys::D);
	// Mouse: negate Y so mouse-up looks up (same as the Third Person template).
	IMC->MapKey(LookAction, EKeys::Mouse2D).Modifiers.Add(Negate(false, true));
	IMC->MapKey(JumpAction, EKeys::SpaceBar);
	IMC->MapKey(SprintAction, EKeys::LeftShift);
	IMC->MapKey(FireAction, EKeys::LeftMouseButton);
	IMC->MapKey(ReloadAction, EKeys::R);
	IMC->MapKey(ShoulderSwapAction, EKeys::Q);
	IMC->MapKey(PauseAction, EKeys::Escape);
	DefaultMappingContext = IMC;
	UE_LOG(LogNightShift, Log, TEXT("ANightShiftCharacter: built runtime Enhanced Input defaults (no Editor assets assigned)."));
}

void ANightShiftCharacter::EnsureMappingContext()
{
	if (!DefaultMappingContext)
	{
		return;
	}
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
	if (Subsystem && !Subsystem->HasMappingContext(DefaultMappingContext))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}

bool ANightShiftCharacter::IsMatchPaused() const
{
	const AArenaGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AArenaGameMode>() : nullptr;
	return GM && GM->IsMatchPaused();
}

void ANightShiftCharacter::Move(const FInputActionValue& Value)
{
	if (!IsAlive() || IsMatchPaused())
	{
		return;
	}
	// WASD — Axis2D
	const FVector2D Axis = Value.Get<FVector2D>();
	if (Controller && (Axis.X != 0.f || Axis.Y != 0.f))
	{
		const FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
		AddMovementInput(Forward, Axis.Y);
		AddMovementInput(Right, Axis.X);
	}
}

void ANightShiftCharacter::Look(const FInputActionValue& Value)
{
	// Only while the match is live: in WaitingToStart / Won / Lost / paused the cursor is free
	// and the first capture delta would otherwise spin the camera.
	const AArenaGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AArenaGameMode>() : nullptr;
	if (GM && (GM->MatchState != EArenaMatchState::InProgress || GM->IsMatchPaused()))
	{
		return;
	}
	// Mouse only — recoil is applied in AddRecoilKick + UpdateRecoilRecovery (not dribbled here).
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void ANightShiftCharacter::StartSprint()
{
	bWantsSprint = true;
	UpdateSprintSpeed();
}

void ANightShiftCharacter::StopSprint()
{
	bWantsSprint = false;
	UpdateSprintSpeed();
}

void ANightShiftCharacter::UpdateSprintSpeed()
{
	if (!GetCharacterMovement())
	{
		return;
	}
	const float Walk = GameConfig ? GameConfig->WalkSpeed : 600.f;
	const float Sprint = GameConfig ? GameConfig->SprintSpeed : 900.f;
	GetCharacterMovement()->MaxWalkSpeed = bWantsSprint ? Sprint : Walk;
}

void ANightShiftCharacter::SwapShoulder()
{
	if (!IsAlive())
	{
		return;
	}
	// Q — shoulder swap (DESIGN: right default)
	bRightShoulder = !bRightShoulder;
	ApplyShoulderOffset();
}

void ANightShiftCharacter::ApplyShoulderOffset()
{
	if (!CameraBoom)
	{
		return;
	}
	const float Y = bRightShoulder ? 60.f : -60.f;
	CameraBoom->SocketOffset = FVector(0.f, Y, 60.f);
}

void ANightShiftCharacter::StartFire()
{
	if (!IsAlive() || IsMatchPaused())
	{
		return;
	}
	if (Rifle)
	{
		Rifle->Fire();
	}
}

void ANightShiftCharacter::StopFire()
{
	if (Rifle)
	{
		Rifle->StopFire();
	}
}

void ANightShiftCharacter::RequestReload()
{
	if (Rifle)
	{
		Rifle->Reload();
	}
}


void ANightShiftCharacter::TryJumpOrMantle()
{
	if (!IsAlive() || IsMatchPaused())
	{
		return;
	}
	if (TryMantleOverLedge())
	{
		return;
	}
	Jump();
}

bool ANightShiftCharacter::TryMantleOverLedge()
{
	// Optional DESIGN mantle: forward probe for waist-high ledge, then up+over. No animation required for v1.
	UWorld* World = GetWorld();
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!World || !Move || !Move->IsMovingOnGround())
	{
		return false;
	}

	const FVector Start = GetActorLocation();
	const FVector Forward = GetActorForwardVector();
	FHitResult WallHit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MantleWall), false, this);
	const FVector WallEnd = Start + Forward * MantleReachCm;
	if (!World->LineTraceSingleByChannel(WallHit, Start, WallEnd, ECC_Visibility, Params))
	{
		return false;
	}

	const FVector TopStart = WallHit.ImpactPoint + Forward * 10.f + FVector(0.f, 0.f, MantleHeightCm);
	const FVector TopEnd = TopStart - FVector(0.f, 0.f, MantleHeightCm + 40.f);
	FHitResult TopHit;
	if (!World->LineTraceSingleByChannel(TopHit, TopStart, TopEnd, ECC_Visibility, Params))
	{
		return false;
	}

	const FVector Landing = TopHit.ImpactPoint + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f);
	SetActorLocation(Landing, false, nullptr, ETeleportType::TeleportPhysics);
	Move->StopMovementImmediately();
	UE_LOG(LogNightShift, Verbose, TEXT("Mantle success"));
	return true;
}

void ANightShiftCharacter::RequestPause()
{
	// Esc toggles pause / unlock (DESIGN) — only during InProgress (PauseMatch enforces)
	if (AArenaGameMode* GM = Cast<AArenaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (GM->MatchState == EArenaMatchState::InProgress || GM->IsMatchPaused())
		{
			GM->PauseMatch(!GM->IsMatchPaused());
		}
	}
}

float ANightShiftCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (!IsAlive())
	{
		return 0.f;
	}
	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	Health = FMath::Max(0.f, Health - Applied);
	TimeSinceLastDamage = 0.f;

	if (Applied > 0.f)
	{
		OnDamaged.Broadcast(Applied);
		// Camera shake (optional class). Vignette: WBP binds to OnDamaged.
		if (DamageCameraShake)
		{
			if (APlayerController* PC = Cast<APlayerController>(GetController()))
			{
				const float Scale = GameConfig ? GameConfig->CameraShakeScale : 0.35f;
				PC->ClientStartCameraShake(DamageCameraShake, Scale);
			}
		}
	}
	BroadcastHealthChanged();

	if (Health <= 0.f)
	{
		OnDied.Broadcast();
		if (AArenaGameMode* GM = Cast<AArenaGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GM->NotifyPlayerDied();
		}
	}
	return Applied;
}

void ANightShiftCharacter::UpdateRegen(float DeltaSeconds)
{
	// DESIGN: regen 10 HP/s after 5 s without damage
	TimeSinceLastDamage += DeltaSeconds;
	const float Delay = GameConfig ? GameConfig->PlayerRegenDelaySeconds : 5.f;
	const float Rate = GameConfig ? GameConfig->PlayerRegenPerSecond : 10.f;
	const float MaxHP = GameConfig ? GameConfig->PlayerMaxHealth : 100.f;
	if (TimeSinceLastDamage >= Delay && Health < MaxHP && Health > 0.f)
	{
		const float Prev = Health;
		Health = FMath::Min(MaxHP, Health + Rate * DeltaSeconds);
		if (!FMath::IsNearlyEqual(Prev, Health))
		{
			BroadcastHealthChanged();
		}
	}
}

void ANightShiftCharacter::ApplyHeal(float Amount)
{
	const float MaxHP = GameConfig ? GameConfig->PlayerMaxHealth : 100.f;
	Health = FMath::Clamp(Health + Amount, 0.f, MaxHP);
	BroadcastHealthChanged();
}

void ANightShiftCharacter::BroadcastHealthChanged()
{
	const float MaxHP = GameConfig ? GameConfig->PlayerMaxHealth : 100.f;
	OnHealthChanged.Broadcast(Health, MaxHP);
}

void ANightShiftCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	const float FallMeters = (LastGroundedZ - GetActorLocation().Z) / 100.f;
	const float Threshold = GameConfig ? GameConfig->FallDamageHeightMeters : 6.f;
	// Formula (ArenaCollision path): ~15 HP per excess meter over threshold (DESIGN >6 m).
	if (ArenaCollision)
	{
		ArenaCollision->ApplyFallDamageIfNeeded(this, FallMeters, Threshold);
	}
	else if (FallMeters > Threshold)
	{
		const float Excess = FallMeters - Threshold;
		TakeDamage(Excess * 15.f, FDamageEvent(), nullptr, this);
	}
	LastGroundedZ = GetActorLocation().Z;
	bWasMovingOnGround = true;
}

void ANightShiftCharacter::AddRecoilKick(float PitchDegrees, float YawDegrees)
{
	// Accumulate residual for recovery tracking
	RecoilOffsetDegrees.X += PitchDegrees;
	RecoilOffsetDegrees.Y += YawDegrees;

	// Immediate kick — snappy gunfeel (DESIGN: small random recoil)
	AddControllerPitchInput(PitchDegrees);
	AddControllerYawInput(YawDegrees);
}

void ANightShiftCharacter::UpdateRecoilRecovery(float DeltaSeconds)
{
	if (RecoilOffsetDegrees.IsNearlyZero())
	{
		RecoilOffsetDegrees = FVector2D::ZeroVector;
		return;
	}

	const float Speed = GameConfig ? GameConfig->RecoilRecoverySpeed : 8.f;
	const FVector2D Prev = RecoilOffsetDegrees;
	RecoilOffsetDegrees = FMath::Vector2DInterpTo(RecoilOffsetDegrees, FVector2D::ZeroVector, DeltaSeconds, Speed);

	// Apply recovered amount as reverse controller input (camera settles toward pre-kick aim)
	const FVector2D Recovered = Prev - RecoilOffsetDegrees;
	if (!Recovered.IsNearlyZero())
	{
		AddControllerPitchInput(-Recovered.X);
		AddControllerYawInput(-Recovered.Y);
	}
}

void ANightShiftCharacter::SoftResetPlayerState()
{
	Health = GameConfig ? GameConfig->PlayerMaxHealth : 100.f;
	TimeSinceLastDamage = GameConfig ? GameConfig->PlayerRegenDelaySeconds : 5.f;
	RecoilOffsetDegrees = FVector2D::ZeroVector;
	bWantsSprint = false;
	UpdateSprintSpeed();
	if (Rifle)
	{
		Rifle->SoftResetAmmo();
	}
	StopFire();
	BroadcastHealthChanged();
}

FVector ANightShiftCharacter::GetAimOrigin() const
{
	return FollowCamera ? FollowCamera->GetComponentLocation() : GetActorLocation();
}

FVector ANightShiftCharacter::GetAimDirection() const
{
	if (FollowCamera)
	{
		return FollowCamera->GetForwardVector();
	}
	return GetActorForwardVector();
}
