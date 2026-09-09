#include "AlienBot.h"
#include "GameConfig.h"
#include "NightShiftCharacter.h"
#include "ArenaCollision.h"
#include "ArenaGameMode.h"
#include "OfficeArena.h"
#include "NightShiftFloor37.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "AIController.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/PointLightComponent.h"
#include "FXPoolInterface.h"
#include "Animation/AnimSequence.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "NightShiftAudio.h"

namespace AlienBotPrivate
{
	/** Forward probe length for simple obstacle steering (cm). */
	constexpr float SteerProbeCm = 180.f;
	/** Blend of lateral offset when probe is blocked (unitless). */
	constexpr float SteerLateralWeight = 0.85f;
}

AAlienBot::AAlienBot()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(40.f, 88.f);
	// Rifle hitscan traces ECC_Visibility; the default Pawn profile ignores that channel,
	// so block it explicitly or shots pass straight through the bot.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetCharacterMovement()->MaxWalkSpeed = 400.f; // 4 m/s
	GetCharacterMovement()->bOrientRotationToMovement = true;
	// Sprint Y — yaw is owned here (orient-to-movement while chasing, FaceTarget in combat), never by
	// the AIController, so the two cannot fight.
	bUseControllerRotationYaw = false;
	ArenaCollision = CreateDefaultSubobject<UArenaCollision>(TEXT("ArenaCollision"));
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Greybox visuals. Capsule 40/88: body cylinder, head sphere in the top 25%.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -22.f));
	BodyMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.3f));
	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(GetCapsuleComponent());
	HeadMesh->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	HeadMesh->SetRelativeScale3D(FVector(0.48f, 0.48f, 0.48f));
	for (UStaticMeshComponent* M : { BodyMesh.Get(), HeadMesh.Get() })
	{
		if (ShapeMat.Succeeded())
		{
			M->SetMaterial(0, ShapeMat.Object);
		}
		M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		M->SetCollisionResponseToAllChannels(ECR_Ignore);
		M->SetCastShadow(true);
	}
	if (CylinderMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CylinderMesh.Object);
	}
	if (SphereMesh.Succeeded())
	{
		HeadMesh->SetStaticMesh(SphereMesh.Object);
	}

	FlashLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FlashLight"));
	FlashLight->SetupAttachment(GetCapsuleComponent());
	FlashLight->SetRelativeLocation(FVector(0.f, 0.f, 30.f));
	FlashLight->Intensity = 0.f;
	FlashLight->AttenuationRadius = 350.f;
	FlashLight->LightColor = FColor(210, 255, 220);
	FlashLight->SetCastShadows(false);
}


void AAlienBot::ApplyConfiguredMeshes()
{
	// Phase 8 prep: ModelFinder assets under Content/Imported → assign on DA_GameConfig.
	// Soft refs default null → keep constructor greybox cylinder/sphere.
	if (!GameConfig)
	{
		return;
	}

	GameConfig->ResolvePhase8LoadedMeshes();

	if (USkeletalMesh* Skel = GameConfig->CachedAlienSkeletalMesh.Get())
	{
		USkeletalMeshComponent* CharMesh = GetMesh();
		const bool bFirstApply = CharMesh && CharMesh->GetSkeletalMeshAsset() != Skel;
		if (bFirstApply)
		{
			InvalidateFlashMIDs();
			EndFlashSwap();
			CharMesh->SetSkeletalMesh(Skel);
			CharMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			// Body blocks Visibility when it has a physics asset (per-body hits + bone names);
			// otherwise the refit capsule is the hit volume.
			CharMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			CharMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
			CharMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			// Sprint AB — with a physics asset the rifle trace must reach the bodies, so the capsule steps
			// aside on Visibility (it still blocks Pawn/Camera and drives movement + soft-lock overlaps).
			bBodyHasPhysicsAsset = Skel->GetPhysicsAsset() != nullptr;
			GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, bBodyHasPhysicsAsset ? ECR_Ignore : ECR_Block);
			CharMesh->SetCastShadow(true);
			FitSkeletalBody(Skel);
			SkelOriginalMaterials.Reset();
		}
		if (CharMesh)
		{
			CharMesh->SetVisibility(true);
			CharMesh->SetHiddenInGame(false);
		}
		if (BodyMesh)
		{
			BodyMesh->SetVisibility(false);
			BodyMesh->SetHiddenInGame(true);
		}
		if (HeadMesh)
		{
			HeadMesh->SetVisibility(false);
			HeadMesh->SetHiddenInGame(true);
		}
		bSkeletalActive = true;
		ApplyFlashToMaterials(); // creates per-slot MIDs (params are a no-op on the atlas material)
		if (CharMesh && SkelOriginalMaterials.Num() == 0)
		{
			for (int32 Slot = 0; Slot < CharMesh->GetNumMaterials(); ++Slot)
			{
				SkelOriginalMaterials.Add(CharMesh->GetMaterial(Slot));
			}
		}
		CurrentAnim = nullptr;
		AttackAnimRemaining = 0.f;
		DeathHideRemaining = 0.f;
		PlayAlienAnim(GameConfig->CachedAlienAnims.Idle, true);
		if (bFirstApply)
		{
			UE_LOG(LogNightShift, Log, TEXT("AAlienBot::ApplyConfiguredMeshes — skeletal %s scale %.2f, capsule r=%.0f h=%.0f, physics asset %s, anims idle=%s run=%s death=%s."),
				*Skel->GetName(), GameConfig->AlienMeshScale,
				GetCapsuleComponent()->GetScaledCapsuleRadius(), GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),
				Skel->GetPhysicsAsset() ? TEXT("yes") : TEXT("no"),
				GameConfig->CachedAlienAnims.Idle ? TEXT("ok") : TEXT("null"),
				GameConfig->CachedAlienAnims.Run ? TEXT("ok") : TEXT("null"),
				GameConfig->CachedAlienAnims.Death ? TEXT("ok") : TEXT("null"));
		}
		return;
	}

	bool bSwapped = false;
	if (BodyMesh)
	{
		if (UStaticMesh* Body = GameConfig->CachedAlienBodyMesh.Get())
		{
			InvalidateFlashMIDs();
			BodyMesh->SetStaticMesh(Body);
			BodyMesh->SetVisibility(true);
			BodyMesh->SetHiddenInGame(false);
			bSwapped = true;
			// Quaternius Alien is a single mesh — hide greybox head sphere unless head override set.
			if (HeadMesh && GameConfig->AlienHeadMesh.IsNull())
			{
				HeadMesh->SetVisibility(false);
				HeadMesh->SetHiddenInGame(true);
			}
		}
	}
	if (HeadMesh && !GameConfig->AlienHeadMesh.IsNull())
	{
		if (UStaticMesh* Head = GameConfig->CachedAlienHeadMesh.Get())
		{
			HeadMID = nullptr;
			HeadMesh->SetStaticMesh(Head);
			HeadMesh->SetVisibility(true);
			HeadMesh->SetHiddenInGame(false);
			bSwapped = true;
		}
	}
	if (bSwapped)
	{
		// Bright bio-readable tint + flash-ready MIDs (Color/BaseColor/Emissive fallbacks).
		ApplyFlashToMaterials();
		UE_LOG(LogNightShift, Log, TEXT("AAlienBot::ApplyConfiguredMeshes — static mesh override(s) + flash MIDs applied."));
	}
}

void AAlienBot::FitSkeletalBody(USkeletalMesh* Skel)
{
	USkeletalMeshComponent* CharMesh = GetMesh();
	if (!CharMesh || !Skel)
	{
		return;
	}
	const float Base = GameConfig ? FMath::Max(GameConfig->AlienMeshScale, 0.05f) : 1.f;
	const FAlienVariantTuning* VT = GetVariantTuning();
	const float ScaleMul = VT ? FMath::Max(VT->ScaleMul, 0.1f) : 1.f;
	const float S = Base * ScaleMul;
	AppliedScaleMul = ScaleMul;
	const float Yaw = GameConfig ? GameConfig->AlienMeshYawDegrees : -90.f;
	const FBoxSphereBounds B = Skel->GetBounds(); // ref-pose bounds in mesh space
	GruntHalfHeight = FMath::Max(B.BoxExtent.Z * Base, 30.f);
	const float HalfH = FMath::Max(B.BoxExtent.Z * S, 30.f);
	const float Radius = FMath::Clamp(FMath::Min(B.BoxExtent.X, B.BoxExtent.Y) * S, 25.f, HalfH * 0.75f);
	GetCapsuleComponent()->SetCapsuleSize(Radius, HalfH);
	// Feet (bounds bottom) sit on the capsule bottom.
	const float MeshBottom = (B.Origin.Z - B.BoxExtent.Z) * S;
	CharMesh->SetRelativeScale3D(FVector(S));
	CharMesh->SetRelativeLocation(FVector(0.f, 0.f, -HalfH - MeshBottom));
	CharMesh->SetRelativeRotation(FRotator(0.f, Yaw, 0.f));
	if (FlashLight)
	{
		FlashLight->SetRelativeLocation(FVector(0.f, 0.f, HalfH * 0.3f));
	}
}

void AAlienBot::SetVariant(EAlienVariant NewVariant)
{
	Variant = NewVariant;
}

const FAlienVariantTuning* AAlienBot::GetVariantTuning() const
{
	if (!GameConfig)
	{
		return nullptr;
	}
	switch (Variant)
	{
	case EAlienVariant::Brute:   return &GameConfig->Brute;
	case EAlienVariant::Stalker: return &GameConfig->Stalker;
	default:                     return nullptr;
	}
}

void AAlienBot::ApplyVariantPresentation()
{
	const FAlienVariantTuning* VT = GetVariantTuning();
	const float WantMul = VT ? FMath::Max(VT->ScaleMul, 0.1f) : 1.f;
	if (bSkeletalActive && !FMath::IsNearlyEqual(WantMul, AppliedScaleMul, 0.001f))
	{
		if (USkeletalMeshComponent* CharMesh = GetMesh())
		{
			if (USkeletalMesh* Skel = CharMesh->GetSkeletalMeshAsset())
			{
				FitSkeletalBody(Skel); // caller (ActivateAtSpawn) re-seats the capsule on the floor
			}
		}
	}
	// Speed: wave-scaled chase speed × variant multiplier.
	const AArenaGameMode* GM = GetArenaGameMode();
	const float Speed = GM ? GM->GetAlienMoveSpeed() : (GameConfig ? GameConfig->AlienMoveSpeed : 400.f);
	GetCharacterMovement()->MaxWalkSpeed = Speed * (VT ? VT->SpeedMul : 1.f);
	// Glow: always-on tinted light for variants; Grunt keeps the plain hit-flash light.
	if (FlashLight)
	{
		GlowBaseIntensity = VT ? VT->GlowIntensity : 0.f;
		FlashLight->SetLightColor(VT ? VT->GlowColor : FLinearColor(FColor(210, 255, 220)));
		FlashLight->SetIntensity(GlowBaseIntensity);
	}
}

void AAlienBot::PlayAlienAnim(UAnimSequence* Seq, bool bLoop, float Rate)
{
	USkeletalMeshComponent* CharMesh = GetMesh();
	if (!CharMesh || !Seq || !bSkeletalActive)
	{
		return;
	}
	if (CurrentAnim == Seq)
	{
		CharMesh->SetPlayRate(Rate);
		return;
	}
	CurrentAnim = Seq;
	CharMesh->PlayAnimation(Seq, bLoop);
	CharMesh->SetPlayRate(Rate);
}

void AAlienBot::UpdateAlienAnim(float DeltaSeconds)
{
	if (!bSkeletalActive || !GameConfig)
	{
		return;
	}
	const FNightShiftAlienAnimCache& A = GameConfig->CachedAlienAnims;
	if (!bIsAlive)
	{
		return; // Death clip plays to its last frame; Tick handles the hide timer
	}
	if (AttackAnimRemaining > 0.f)
	{
		AttackAnimRemaining -= DeltaSeconds;
		if (AttackAnimRemaining > 0.f)
		{
			return;
		}
		CurrentAnim = nullptr;
	}
	const float Speed = GetVelocity().Size2D();
	switch (CombatState)
	{
	case EAlienCombatState::Chase:
	{
		const float Ref = FMath::Max(GameConfig->AlienRunAnimRefSpeed, 1.f);
		if (Speed < 20.f) { PlayAlienAnim(A.Idle, true); }
		else { PlayAlienAnim(A.Run ? A.Run.Get() : A.Walk.Get(), true, FMath::Clamp(Speed / Ref, 0.6f, 1.8f)); }
		break;
	}
	case EAlienCombatState::StrafeBurst:
		if (Speed < 20.f) { PlayAlienAnim(A.Idle, true); }
		else { PlayAlienAnim(A.Walk ? A.Walk.Get() : A.Run.Get(), true, FMath::Clamp(Speed / 180.f, 0.6f, 1.6f)); }
		break;
	default:
		PlayAlienAnim(A.Idle, true);
		break;
	}
}

void AAlienBot::BeginFlashSwap()
{
	USkeletalMeshComponent* CharMesh = GetMesh();
	if (!bSkeletalActive || !CharMesh || bFlashSwapActive)
	{
		return;
	}
	// Sprint AB — the Mutant's M_AlienVariant has real Tint/Emissive params; the white material swap is
	// only for the Quaternius atlas, which ignores parameters.
	if (GameConfig && GameConfig->bUsingMutantModel)
	{
		return;
	}
	if (!FlashSwapMID)
	{
		UMaterialInterface* Base = BodyMesh ? BodyMesh->GetMaterial(0) : nullptr;
		if (Base)
		{
			FlashSwapMID = UMaterialInstanceDynamic::Create(Base, this);
			FlashSwapMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 1.f, 1.f));
		}
	}
	if (!FlashSwapMID)
	{
		return;
	}
	if (SkelOriginalMaterials.Num() == 0)
	{
		for (int32 Slot = 0; Slot < CharMesh->GetNumMaterials(); ++Slot)
		{
			SkelOriginalMaterials.Add(CharMesh->GetMaterial(Slot));
		}
	}
	for (int32 Slot = 0; Slot < CharMesh->GetNumMaterials(); ++Slot)
	{
		CharMesh->SetMaterial(Slot, FlashSwapMID);
	}
	bFlashSwapActive = true;
}

void AAlienBot::EndFlashSwap()
{
	if (!bFlashSwapActive)
	{
		return;
	}
	bFlashSwapActive = false;
	if (USkeletalMeshComponent* CharMesh = GetMesh())
	{
		for (int32 Slot = 0; Slot < SkelOriginalMaterials.Num() && Slot < CharMesh->GetNumMaterials(); ++Slot)
		{
			CharMesh->SetMaterial(Slot, SkelOriginalMaterials[Slot]);
		}
	}
}

void AAlienBot::BeginPlay()
{
	Super::BeginPlay();
	ApplyFlashToMaterials(); // paints the greybox in BodyColor
	if (GameConfig)
	{
		GetCharacterMovement()->MaxWalkSpeed = GameConfig->AlienMoveSpeed;
		GetCapsuleComponent()->SetCapsuleSize(GameConfig->AlienCapsuleRadiusCm, GameConfig->AlienCapsuleHalfHeightCm);
		if (ArenaCollision)
		{
			ArenaCollision->GameConfig = GameConfig;
		}
	}
	ApplyConfiguredMeshes(); // Phase 8 soft refs — no-op when unset
	BurstCooldownRemaining = 0.f;
	BurstShotsRemaining = 0;
	BurstIntraShotRemaining = 0.f;
}

void AAlienBot::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float MaxDt = GameConfig ? GameConfig->MaxDeltaTimeClampSeconds : 0.05f;
	if (DeltaSeconds > MaxDt)
	{
		DeltaSeconds = MaxDt;
	}

	// Esc pause freezes bots (DESIGN: pause / unlock). The movement component keeps simulating
	// on its own tick, so kill residual velocity too or bots drift a few cm while paused.
	if (const AArenaGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AArenaGameMode>() : nullptr)
	{
		if (GM->IsMatchPaused())
		{
			if (UCharacterMovementComponent* Move = GetCharacterMovement())
			{
				Move->StopMovementImmediately();
			}
			return;
		}
	}

	UpdateHitFlash(DeltaSeconds);

	if (!bIsAlive)
	{
		if (DeathHideRemaining > 0.f)
		{
			DeathHideRemaining -= DeltaSeconds;
			if (DeathHideRemaining <= 0.f)
			{
				SetActorHiddenInGame(true);
			}
		}
		return;
	}

	if (ArenaCollision)
	{
		ArenaCollision->PushApartNearbyAliens(this);
	}

	HitReactCooldownRemaining = FMath::Max(0.f, HitReactCooldownRemaining - DeltaSeconds);
	if (HitReactRemaining > 0.f)
	{
		// Sprint AB — staggered: hold position, drop any burst, keep facing whatever we were facing.
		HitReactRemaining -= DeltaSeconds;
		BurstShotsRemaining = 0;
		BurstIntraShotRemaining = 0.f;
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			Move->StopMovementImmediately();
		}
		if (HitReactRemaining <= 0.f)
		{
			CurrentAnim = nullptr; // force the locomotion clip to restart cleanly
		}
		return;
	}

	UpdateAI(DeltaSeconds);
	UpdateAlienAnim(DeltaSeconds);
}

void AAlienBot::TryHitReact()
{
	if (!bIsAlive || !bSkeletalActive || !GameConfig || !GameConfig->bAlienHitReact)
	{
		return;
	}
	UAnimSequence* React = GameConfig->CachedAlienAnims.HitReact.Get();
	if (!React || HitReactRemaining > 0.f || HitReactCooldownRemaining > 0.f)
	{
		return;
	}
	const float Hold = FMath::Max(GameConfig->AlienHitReactSeconds, 0.1f);
	const float Rate = FMath::Max(React->GetPlayLength() / Hold, 0.5f);
	CurrentAnim = nullptr;
	AttackAnimRemaining = 0.f;
	PlayAlienAnim(React, false, Rate);
	HitReactRemaining = Hold;
	HitReactCooldownRemaining = Hold + FMath::Max(GameConfig->AlienHitReactCooldownSeconds, 0.f);
}

void AAlienBot::InvalidateFlashMIDs()
{
	BodyMID = nullptr;
	HeadMID = nullptr;
	SkelMIDs.Reset();
}

void AAlienBot::ApplyBioFlashColorToMID(UMaterialInstanceDynamic* MID, const FLinearColor& Color) const
{
	if (!MID)
	{
		return;
	}
	// Engine BasicShape uses "Color"; imported Quaternius/Atlas mats often use BaseColor.
	static const FName VectorNames[] = {
		TEXT("Color"), TEXT("BaseColor"), TEXT("Tint"), TEXT("DiffuseColor")
	};
	for (const FName& Name : VectorNames)
	{
		MID->SetVectorParameterValue(Name, Color);
	}
	// Flash readability on mats without a Color pin — drive emissive if present.
	const float Flash = FMath::Clamp(HitFlashAlpha, 0.f, 1.f);
	const FLinearColor Emissive = FLinearColor(1.f, 1.f, 1.f) * (Flash * 8.f) + BodyColor * 0.35f;
	MID->SetVectorParameterValue(TEXT("EmissiveColor"), Emissive);
	MID->SetVectorParameterValue(TEXT("Emissive"), Emissive);
	MID->SetScalarParameterValue(TEXT("EmissiveStrength"), Flash * 8.f + 0.35f);
}

void AAlienBot::ApplyFlashToMaterials()
{
	const float Flash = FMath::Clamp(HitFlashAlpha, 0.f, 1.f);
	const FLinearColor C = FMath::Lerp(BodyColor, FLinearColor::White, Flash);

	// Static greybox / SM_Alien body
	if (BodyMesh && !BodyMesh->bHiddenInGame && BodyMesh->IsVisible())
	{
		if (!BodyMID)
		{
			BodyMID = BodyMesh->CreateAndSetMaterialInstanceDynamic(0);
		}
		ApplyBioFlashColorToMID(BodyMID, C);
	}
	if (HeadMesh && !HeadMesh->bHiddenInGame && HeadMesh->IsVisible())
	{
		if (!HeadMID)
		{
			HeadMID = HeadMesh->CreateAndSetMaterialInstanceDynamic(0);
		}
		ApplyBioFlashColorToMID(HeadMID, C);
	}

	// Skeletal path (AlienSkeletalMesh) — all material slots. Sprint AB: on M_AlienVariant the Tint /
	// EmissiveColor / EmissiveStrength parameters are real, so variants read on the skin itself.
	if (USkeletalMeshComponent* CharMesh = GetMesh())
	{
		if (CharMesh->GetSkeletalMeshAsset() && !CharMesh->bHiddenInGame && CharMesh->IsVisible())
		{
			if (SkelMIDs.Num() == 0)
			{
				const int32 NumMats = CharMesh->GetNumMaterials();
				SkelMIDs.Reserve(NumMats);
				for (int32 Slot = 0; Slot < NumMats; ++Slot)
				{
					SkelMIDs.Add(CharMesh->CreateAndSetMaterialInstanceDynamic(Slot));
				}
			}
			const FAlienVariantTuning* VT = GetVariantTuning();
			const FLinearColor BaseTint = VT ? VT->Tint : (GameConfig ? GameConfig->AlienGruntTint : FLinearColor::White);
			const FLinearColor SkinTint = FMath::Lerp(BaseTint, FLinearColor::White, Flash);
			const FLinearColor Emissive = FLinearColor::White * (Flash * 8.f) + (VT ? VT->GlowColor * VT->RestingEmissive : FLinearColor::Black);
			const float EmissiveStrength = Flash * 8.f + (VT ? VT->RestingEmissive : 0.f);
			for (UMaterialInstanceDynamic* MID : SkelMIDs)
			{
				if (!MID)
				{
					continue;
				}
				MID->SetVectorParameterValue(TEXT("Tint"), SkinTint);
				MID->SetVectorParameterValue(TEXT("Color"), SkinTint);
				MID->SetVectorParameterValue(TEXT("BaseColor"), SkinTint);
				MID->SetVectorParameterValue(TEXT("EmissiveColor"), Emissive);
				MID->SetScalarParameterValue(TEXT("EmissiveStrength"), EmissiveStrength);
			}
		}
	}

	// Point light pop — stronger so flash reads even when mat params ignore Color. Variants glow at rest.
	if (FlashLight)
	{
		FlashLight->SetIntensity(GlowBaseIntensity + Flash * 1400.f);
	}
}

void AAlienBot::UpdateHitFlash(float DeltaSeconds)
{
	if (HitFlashTimeRemaining <= 0.f)
	{
		if (HitFlashAlpha > 0.f)
		{
			HitFlashAlpha = 0.f;
			ApplyFlashToMaterials();
		}
		EndFlashSwap();
		return;
	}
	HitFlashTimeRemaining -= DeltaSeconds;
	const float DurSec = (GameConfig ? GameConfig->HitFlashDurationMs : 80.f) * 0.001f;
	// HitFlashAlpha 1→0 over ~80 ms for BP/material (no mesh materials required in C++).
	HitFlashAlpha = DurSec > KINDA_SMALL_NUMBER
		? FMath::Clamp(HitFlashTimeRemaining / DurSec, 0.f, 1.f)
		: 0.f;
	if (HitFlashTimeRemaining <= 0.f)
	{
		HitFlashTimeRemaining = 0.f;
		HitFlashAlpha = 0.f;
		if (bIsFlashing)
		{
			bIsFlashing = false;
			OnHitFlash.Broadcast(false);
		}
	}
	ApplyFlashToMaterials();
}

void AAlienBot::SetTarget(ANightShiftCharacter* InTarget)
{
	TargetPlayer = InTarget;
}

void AAlienBot::ActivateAtSpawn(const FTransform& SpawnTransform)
{
	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
	bRespawnScheduled = false;
	SetActorTransform(SpawnTransform);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->SetMovementMode(MOVE_Walking);
		Move->StopMovementImmediately();
	}
	bIsAlive = true;
	BodyHitCount = 0;
	HeadHitCount = 0;
	HitReactRemaining = 0.f;
	HitReactCooldownRemaining = 0.f;
	CombatState = EAlienCombatState::Chase;
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = true;
	}
	SteerCommitRemaining = 0.f;
	ApplyConfiguredMeshes();
	ApplyVariantPresentation();
	if (GameConfig)
	{
		const AArenaGameMode* GM = GetArenaGameMode();
		if (GM && GM->MatchState == EArenaMatchState::InProgress)
		{
			NightShiftAudio::PlayAt(this, GameConfig->CachedSoundAlienGrowl, GetActorLocation() + FVector(0.f, 0.f, 60.f), GameConfig->SfxVolume * 0.6f, 0.15f);
		}
	}
	// Spawn transforms assume the Grunt capsule; re-seat so this variant's feet touch the same floor.
	{
		const float Half = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		FVector Loc = SpawnTransform.GetLocation();
		Loc.Z += Half - GruntHalfHeight;
		SetActorLocation(Loc, false, nullptr, ETeleportType::TeleportPhysics);
	}
	BurstCooldownRemaining = 0.f;
	BurstShotsRemaining = 0;
	BurstIntraShotRemaining = 0.f;
	HitFlashTimeRemaining = 0.f;
	HitFlashAlpha = 0.f;
	if (bIsFlashing)
	{
		bIsFlashing = false;
		OnHitFlash.Broadcast(false);
	}
}

void AAlienBot::SoftDespawn()
{
	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
	bRespawnScheduled = false;
	bIsAlive = false;
	CombatState = EAlienCombatState::Dead;
	BurstShotsRemaining = 0;
	BurstIntraShotRemaining = 0.f;
	BurstCooldownRemaining = 0.f;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	DeathHideRemaining = 0.f;
	EndFlashSwap();
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
	}
}

void AAlienBot::SoftReset()
{
	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
	bRespawnScheduled = false;
	BodyHitCount = 0;
	HeadHitCount = 0;
	BurstCooldownRemaining = 0.f;
	BurstShotsRemaining = 0;
	BurstIntraShotRemaining = 0.f;
	StrafeSign = 1.f;
	SteerSideSign = 1.f;
	SteerCommitRemaining = 0.f;
	HitFlashTimeRemaining = 0.f;
	HitFlashAlpha = 0.f;
	if (bIsFlashing)
	{
		bIsFlashing = false;
		OnHitFlash.Broadcast(false);
	}
	SoftDespawn();
}

AArenaGameMode* AAlienBot::GetArenaGameMode() const
{
	if (!CachedGameMode.IsValid())
	{
		CachedGameMode = Cast<AArenaGameMode>(UGameplayStatics::GetGameMode(this));
	}
	return CachedGameMode.Get();
}

float AAlienBot::FaceTarget(float DeltaSeconds)
{
	if (!TargetPlayer.IsValid())
	{
		return 180.f;
	}
	const FVector ToPlayer = (TargetPlayer->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (ToPlayer.IsNearlyZero())
	{
		return 0.f;
	}
	const float WantYaw = ToPlayer.Rotation().Yaw;
	const float Rate = GameConfig ? GameConfig->AlienFaceTargetTurnRateDegPerSec : 540.f;
	const float NewYaw = FMath::FixedTurn(GetActorRotation().Yaw, WantYaw, Rate * DeltaSeconds);
	SetActorRotation(FRotator(0.f, NewYaw, 0.f));
	return FMath::Abs(FMath::FindDeltaAngleDegrees(NewYaw, WantYaw));
}

void AAlienBot::UpdateAI(float DeltaSeconds)
{
	if (!TargetPlayer.IsValid())
	{
		if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			TargetPlayer = Cast<ANightShiftCharacter>(P);
		}
	}
	if (!TargetPlayer.IsValid())
	{
		CombatState = EAlienCombatState::Idle;
		return;
	}

	// Sprint C — spawn grace: never fire; optionally block chase (Idle) when bSpawnGraceBlocksAlienAggro.
	if (const AArenaGameMode* GM = GetArenaGameMode())
	{
		if (GM->IsSpawnGraceActive())
		{
			BurstShotsRemaining = 0;
			BurstIntraShotRemaining = 0.f;
			const bool bBlockAggro = GameConfig ? GameConfig->bSpawnGraceBlocksAlienAggro : true;
			if (bBlockAggro)
			{
				CombatState = EAlienCombatState::Idle;
				if (UCharacterMovementComponent* Move = GetCharacterMovement())
				{
					Move->StopMovementImmediately();
				}
				return;
			}
			// Aggro allowed during grace: chase only — no StrafeAndBurst / fire.
			CombatState = EAlienCombatState::Chase;
			ChasePlayer(DeltaSeconds);
			return;
		}
	}

	// Sprint V — post-grace fire lock: chase OK, no burst progress.
	if (const AArenaGameMode* GMFire = GetArenaGameMode())
	{
		if (GMFire->IsAlienFireLocked() && !GMFire->IsSpawnGraceActive())
		{
			BurstShotsRemaining = 0;
			BurstIntraShotRemaining = 0.f;
		}
	}

	const float Range = GameConfig ? GameConfig->AlienCombatRangeMeters : 12.f;
	const float Dist = DistanceToTargetMeters();
	BurstCooldownRemaining = FMath::Max(0.f, BurstCooldownRemaining - DeltaSeconds);

	if (Dist <= Range && HasLineOfSightToTarget())
	{
		CombatState = EAlienCombatState::StrafeBurst;
		StrafeAndBurst(DeltaSeconds);
	}
	else
	{
		// Leaving combat range cancels an in-progress burst; cooldown keeps pacing honest.
		BurstShotsRemaining = 0;
		BurstIntraShotRemaining = 0.f;
		CombatState = EAlienCombatState::Chase;
		ChasePlayer(DeltaSeconds);
	}
}

bool AAlienBot::TryNavMeshMoveToTarget()
{
	// Stub for Editor NavMesh: requires AIController + NavMeshBounds in level (see NAVMESH_NOTES.md).
	if (!bPreferNavMeshMoveTo || !TargetPlayer.IsValid())
	{
		return false;
	}
	AAIController* AIC = Cast<AAIController>(GetController());
	if (!AIC)
	{
		return false;
	}
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys)
	{
		return false;
	}
	FAIMoveRequest Req(TargetPlayer.Get());
	Req.SetAcceptanceRadius(100.f);
	Req.SetUsePathfinding(true);
	const FPathFollowingRequestResult Result = AIC->MoveTo(Req);
	return Result.Code == EPathFollowingRequestResult::RequestSuccessful
		|| Result.Code == EPathFollowingRequestResult::AlreadyAtGoal;
}

void AAlienBot::ChasePlayer(float DeltaSeconds)
{
	(void)DeltaSeconds;
	if (!TargetPlayer.IsValid() || !GetWorld())
	{
		return;
	}

	// Prefer NavMesh MoveTo when enabled (atrium stairs/ramps). Else simple steering fallback.
	if (TryNavMeshMoveToTarget())
	{
		return;
	}

	// Chasing: body follows velocity again (combat range hands yaw to FaceTarget).
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = true;
	}

	const FVector ToPlayer = TargetPlayer->GetActorLocation() - GetActorLocation();
	FVector Desired = ToPlayer.GetSafeNormal2D();
	if (Desired.IsNearlyZero())
	{
		Desired = ToPlayer.GetSafeNormal();
	}

	// Simple obstacle steering: forward line trace; if blocked, add a lateral offset. Sprint Y — the
	// side is chosen by probing left/right once and then held for AlienSteerCommitSeconds, instead of
	// flipping every frame (which made bots shiver against cover).
	SteerCommitRemaining = FMath::Max(0.f, SteerCommitRemaining - DeltaSeconds);
	const FVector ProbeStart = GetActorLocation();
	const FVector ProbeEnd = ProbeStart + Desired * AlienBotPrivate::SteerProbeCm;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(AlienSteer), false, this);
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
		SteerHitScratch, ProbeStart, ProbeEnd, ECC_Visibility, Params);

	if (bBlocked && SteerHitScratch.GetActor() != TargetPlayer.Get())
	{
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Desired).GetSafeNormal();
		if (SteerCommitRemaining <= 0.f)
		{
			// Probe both sides at 60°; keep the clearer one (farther hit / no hit).
			auto SideClearance = [&](float Sign) -> float
			{
				const FVector Dir = (Desired + Right * Sign * 1.7f).GetSafeNormal(); // ≈60° off axis
				const FVector End = ProbeStart + Dir * AlienBotPrivate::SteerProbeCm;
				const bool bHit = GetWorld()->LineTraceSingleByChannel(SteerHitScratch, ProbeStart, End, ECC_Visibility, Params);
				return (bHit && SteerHitScratch.GetActor() != TargetPlayer.Get()) ? SteerHitScratch.Distance : AlienBotPrivate::SteerProbeCm * 2.f;
			};
			const float RightClear = SideClearance(1.f);
			const float LeftClear = SideClearance(-1.f);
			if (!FMath::IsNearlyEqual(RightClear, LeftClear, 5.f))
			{
				SteerSideSign = (RightClear > LeftClear) ? 1.f : -1.f;
			}
			SteerCommitRemaining = GameConfig ? GameConfig->AlienSteerCommitSeconds : 0.6f;
		}
		Desired = (Desired + Right * SteerSideSign * AlienBotPrivate::SteerLateralWeight).GetSafeNormal();
	}

	AddMovementInput(Desired, 1.f);
}

void AAlienBot::StrafeAndBurst(float DeltaSeconds)
{
	if (!TargetPlayer.IsValid())
	{
		return;
	}

	// Sprint Y — in combat range the body faces the player (was: orient-to-movement, so strafing
	// bots shot sideways out of their hip). Chase hands yaw back to the movement component.
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = false;
	}
	const float YawError = FaceTarget(DeltaSeconds);

	// Stop forward, strafe L/R (DESIGN). Sprint Y — plant while the attack clip / burst plays so the
	// punch reads and the Walk↔Attack clip swap happens standing still, not mid-slide.
	const bool bPlant = (GameConfig ? GameConfig->bAlienPlantsDuringBurst : true)
		&& (BurstShotsRemaining > 0 || AttackAnimRemaining > 0.f);
	if (!bPlant)
	{
		const FVector ToPlayer = (TargetPlayer->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		const FVector Right = FVector::CrossProduct(FVector::UpVector, ToPlayer).GetSafeNormal();
		AddMovementInput(Right, StrafeSign);
	}

	const AArenaGameMode* GM = GetArenaGameMode();
	const int32 BurstCount = GameConfig ? GameConfig->AlienBurstRoundCount : 3;
	const FAlienVariantTuning* VT = GetVariantTuning();
	const float BurstInterval = (GM ? GM->GetAlienBurstInterval() : (GameConfig ? GameConfig->AlienBurstIntervalSeconds : 1.5f))
		* (VT ? FMath::Max(VT->BurstIntervalMul, 0.1f) : 1.f);
	const float IntraDelay = GameConfig ? GameConfig->AlienBurstIntraShotDelaySeconds : 0.09f;
	const float FacingTol = GameConfig ? GameConfig->AlienFireFacingToleranceDegrees : 25.f;

	// Start a new burst when cooldown is done, no shots are queued, and the bot is looking at the player.
	if (BurstCooldownRemaining <= 0.f && BurstShotsRemaining <= 0 && YawError <= FacingTol)
	{
		BurstShotsRemaining = BurstCount;
		BurstIntraShotRemaining = 0.f; // first shot fires this frame (after tick delay below)
		StrafeSign *= -1.f;
		if (GameConfig && FMath::FRand() < 0.5f)
		{
			NightShiftAudio::PlayAt(this, GameConfig->CachedSoundAlienGrowl, GetActorLocation() + FVector(0.f, 0.f, 60.f),
				GameConfig->SfxVolume * 0.9f, Variant == EAlienVariant::Brute ? 0.f : 0.12f);
		}
		if (bSkeletalActive && GameConfig && GameConfig->CachedAlienAnims.Attack)
		{
			CurrentAnim = nullptr;
			PlayAlienAnim(GameConfig->CachedAlienAnims.Attack, false, 1.2f);
			AttackAnimRemaining = GameConfig->CachedAlienAnims.Attack->GetPlayLength() / 1.2f;
		}
	}

	// Fire remaining burst shots with short intra-burst delay — never dump all 3 in one Tick.
	if (BurstShotsRemaining > 0)
	{
		BurstIntraShotRemaining -= DeltaSeconds;
		if (BurstIntraShotRemaining <= 0.f)
		{
			TryBurstShot();
			--BurstShotsRemaining;
			if (BurstShotsRemaining > 0)
			{
				BurstIntraShotRemaining = IntraDelay;
			}
			else
			{
				// Burst complete — wait full interval before the next 3-round volley.
				BurstCooldownRemaining = BurstInterval;
				BurstIntraShotRemaining = 0.f;
			}
		}
	}
}

void AAlienBot::TryBurstShot()
{
	if (!TargetPlayer.IsValid())
	{
		return;
	}
	// Defense-in-depth: never fire during spawn grace (even if chase-only path misroutes).
	const AArenaGameMode* GM = GetArenaGameMode();
	if (GM && GM->IsAlienFireLocked())
	{
		return;
	}
	// Sprint Y — accuracy follows the difficulty ramp (10 % → 35 %); flat DESIGN 30 % when the ramp is off.
	const FAlienVariantTuning* VT = GetVariantTuning();
	const float Accuracy = FMath::Clamp((GM ? GM->GetAlienAccuracy() : (GameConfig ? GameConfig->AlienAccuracy : 0.3f))
		+ (VT ? VT->AccuracyBonus : 0.f), 0.f, 1.f);
	const bool bHit = FMath::FRand() <= Accuracy;

	// Tracer + muzzle light from the bot's muzzle to where the shot went (misses scatter around the player).
	const FVector Muzzle = GetActorLocation() + GetActorForwardVector() * 45.f + FVector(0.f, 0.f, 30.f);
	FVector End = TargetPlayer->GetActorLocation() + FVector(0.f, 0.f, 40.f);
	if (!bHit)
	{
		End += FVector(FMath::FRandRange(-150.f, 150.f), FMath::FRandRange(-150.f, 150.f), FMath::FRandRange(-60.f, 120.f));
	}
	if (!FXPool.IsValid())
	{
		TActorIterator<AFXPoolManager> It(GetWorld());
		if (It)
		{
			FXPool = *It;
		}
	}
	if (FXPool.IsValid())
	{
		FXPool->ActivateTracer(Muzzle, End, GameConfig ? GameConfig->TracerDurationMs : 60.f);
		FXPool->ActivateMuzzleLight(Muzzle, GameConfig ? GameConfig->MuzzleFlashDurationMs : 40.f);
	}
	if (GameConfig)
	{
		NightShiftAudio::PlayAt(this, GameConfig->CachedSoundAlienBolt, Muzzle, GameConfig->SfxVolume * 0.7f, 0.08f);
	}

	if (!bHit)
	{
		return; // miss
	}
	const float Dmg = GameConfig ? GameConfig->AlienDamagePerHit : 10.f;
	UGameplayStatics::ApplyDamage(TargetPlayer.Get(), Dmg, GetController(), this, UDamageType::StaticClass());
}

bool AAlienBot::HasLineOfSightToTarget() const
{
	if (!TargetPlayer.IsValid() || !GetWorld())
	{
		return false;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(AlienLOS), false, this);
	const FVector Start = GetActorLocation() + FVector(0, 0, 60);
	const FVector End = TargetPlayer->GetActorLocation() + FVector(0, 0, 60);
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
		LosHitScratch, Start, End, ECC_Visibility, Params);
	return !bBlocked || LosHitScratch.GetActor() == TargetPlayer.Get();
}

float AAlienBot::DistanceToTargetMeters() const
{
	if (!TargetPlayer.IsValid())
	{
		return TNumericLimits<float>::Max();
	}
	return FVector::Dist(GetActorLocation(), TargetPlayer->GetActorLocation()) / 100.f;
}

float AAlienBot::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (!bIsAlive)
	{
		return 0.f;
	}
	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// Classify head vs body via damage / bone when available
	bool bHead = false;
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent* Point = (const FPointDamageEvent*)&DamageEvent;
		bHead = IsLocationOnHead(Point->HitInfo.ImpactPoint) || IsHeadBone(Point->HitInfo.BoneName);
	}

	if (bHead)
	{
		++HeadHitCount;
	}
	else
	{
		++BodyHitCount;
	}

	PlayHitFlash();
	if (GameConfig)
	{
		FVector HitLoc = GetActorLocation();
		if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
		{
			HitLoc = ((const FPointDamageEvent*)&DamageEvent)->HitInfo.ImpactPoint;
		}
		NightShiftAudio::PlayAt(this, GameConfig->CachedSoundHitFlesh, HitLoc, GameConfig->SfxVolume * (bHead ? 1.f : 0.8f), 0.1f);
	}

	const FAlienVariantTuning* VT = GetVariantTuning();
	const int32 BodyNeed = VT ? FMath::Max(VT->BodyHitsToKill, 1) : (GameConfig ? GameConfig->AlienBodyHitsToKill : 3);
	const int32 HeadNeed = VT ? FMath::Max(VT->HeadshotsToKill, 1) : (GameConfig ? GameConfig->AlienHeadshotsToKill : 2);
	if (HeadHitCount >= HeadNeed || BodyHitCount >= BodyNeed)
	{
		Die();
	}
	else
	{
		TryHitReact();
	}
	return Applied;
}

void AAlienBot::PlayHitFlash()
{
	// DESIGN 80ms white flash: bind mesh material emissive to HitFlashAlpha / OnHitFlash (Editor MID).
	const float Ms = GameConfig ? GameConfig->HitFlashDurationMs : 80.f;
	HitFlashTimeRemaining = Ms * 0.001f;
	HitFlashAlpha = 1.f;
	ApplyFlashToMaterials();
	BeginFlashSwap();
	if (!bIsFlashing)
	{
		bIsFlashing = true;
		OnHitFlash.Broadcast(true);
	}
}

void AAlienBot::Die()
{
	UE_LOG(LogNightShift, Verbose, TEXT("%s died (%s, body %d head %d)."), *GetName(),
		Variant == EAlienVariant::Brute ? TEXT("Brute") : Variant == EAlienVariant::Stalker ? TEXT("Stalker") : TEXT("Grunt"),
		BodyHitCount, HeadHitCount);
	bIsAlive = false;
	CombatState = EAlienCombatState::Dead;
	BurstShotsRemaining = 0;
	BurstIntraShotRemaining = 0.f;
	HitReactRemaining = 0.f;
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
	}
	if (GameConfig)
	{
		const FAlienVariantTuning* DeathVT = GetVariantTuning();
		const float Pitch = DeathVT ? (Variant == EAlienVariant::Brute ? 0.75f : 1.25f) : 1.f;
		NightShiftAudio::PlayAt(this, GameConfig->CachedSoundAlienDeath, GetActorLocation(), GameConfig->SfxVolume, 0.f);
		(void)Pitch;
	}
	// Skeletal body: play the Death clip and stay visible until it ends (or just before respawn).
	SetActorEnableCollision(false);
	EndFlashSwap();
	UAnimSequence* DeathAnim = (bSkeletalActive && GameConfig) ? GameConfig->CachedAlienAnims.Death.Get() : nullptr;
	if (DeathAnim)
	{
		CurrentAnim = nullptr;
		AttackAnimRemaining = 0.f;
		// Fit the clip into the respawn window (Mixamo "Dying" is 4.6 s) instead of hiding it mid-fall.
		const float Respawn = GameConfig ? GameConfig->AlienRespawnSeconds : 3.f;
		const float Window = FMath::Max(Respawn - 0.25f, 0.3f);
		const float Rate = FMath::Max(1.f, DeathAnim->GetPlayLength() / Window);
		PlayAlienAnim(DeathAnim, false, Rate);
		DeathHideRemaining = FMath::Clamp(DeathAnim->GetPlayLength() / Rate, 0.3f, Window);
	}
	else
	{
		SetActorHiddenInGame(true);
	}

	if (AArenaGameMode* GM = Cast<AArenaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->RegisterKill(this);
	}
	ScheduleRespawn();
}

void AAlienBot::ScheduleRespawn()
{
	const float Delay = GameConfig ? GameConfig->AlienRespawnSeconds : 3.f;
	bRespawnScheduled = true;
	GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AAlienBot::PerformRespawn, Delay, false);
}

void AAlienBot::PerformRespawn()
{
	bRespawnScheduled = false;
	// Prefer GameMode pool helper (keeps MaxLiveAliens accounting); fall back to arena spawn API.
	if (AArenaGameMode* GM = Cast<AArenaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (GM->RespawnAlien(this))
		{
			return;
		}
	}

	if (AOfficeArena* Arena = FindArena())
	{
		const FTransform Spawn = Arena->GetFarthestSpawnFrom(GetPlayerLocationOrSelf());
		ActivateAtSpawn(Spawn);
		return;
	}

	UE_LOG(LogNightShift, Warning, TEXT("AlienBot::PerformRespawn — no Arena/GameMode; activating in place."));
	ActivateAtSpawn(GetActorTransform());
}

AOfficeArena* AAlienBot::FindArena() const
{
	if (AArenaGameMode* GM = Cast<AArenaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (AOfficeArena* Arena = GM->GetOfficeArena())
		{
			return Arena;
		}
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	TActorIterator<AOfficeArena> It(World);
	return It ? *It : nullptr;
}

FVector AAlienBot::GetPlayerLocationOrSelf() const
{
	if (TargetPlayer.IsValid())
	{
		return TargetPlayer->GetActorLocation();
	}
	if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		return P->GetActorLocation();
	}
	return GetActorLocation();
}


bool AAlienBot::IsRespawnPending() const
{
	return bRespawnScheduled;
}

bool AAlienBot::IsLocationOnHead(const FVector& WorldLocation) const
{
	// DESIGN: head = top 25% of capsule.
	// Capsule spans [-HalfH, +HalfH] in actor Z; top Frac is Z >= HalfH * (1 - 2*Frac).
	const float HalfH = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float Frac = GameConfig ? GameConfig->AlienHeadFraction : 0.25f;
	const float LocalZ = WorldLocation.Z - GetActorLocation().Z;
	return LocalZ >= HalfH * (1.f - 2.f * Frac);
}

bool AAlienBot::IsHeadBone(FName BoneName) const
{
	if (BoneName.IsNone())
	{
		return false;
	}
	const FString S = BoneName.ToString();
	return S.Equals(TEXT("head"), ESearchCase::IgnoreCase)
		|| S.Equals(TEXT("head_01"), ESearchCase::IgnoreCase)
		|| S.Contains(TEXT("head"), ESearchCase::IgnoreCase);
}
