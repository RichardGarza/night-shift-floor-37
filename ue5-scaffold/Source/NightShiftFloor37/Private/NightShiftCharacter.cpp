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
}

void ANightShiftCharacter::BeginPlay()
{
	Super::BeginPlay();

	ApplyConfigToMovement();
	ConfigureCapsuleFromConfig();
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

	// Add Enhanced Input mapping context
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
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
			EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
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

void ANightShiftCharacter::Move(const FInputActionValue& Value)
{
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

void ANightShiftCharacter::RequestPause()
{
	// Esc toggles pause / unlock (DESIGN)
	if (AArenaGameMode* GM = Cast<AArenaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->PauseMatch(!GM->IsMatchPaused());
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
