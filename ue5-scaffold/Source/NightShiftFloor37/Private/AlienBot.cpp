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
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "AIController.h"
#include "EngineUtils.h"

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
	GetCharacterMovement()->MaxWalkSpeed = 400.f; // 4 m/s
	GetCharacterMovement()->bOrientRotationToMovement = true;
	ArenaCollision = CreateDefaultSubobject<UArenaCollision>(TEXT("ArenaCollision"));
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AAlienBot::BeginPlay()
{
	Super::BeginPlay();
	if (GameConfig)
	{
		GetCharacterMovement()->MaxWalkSpeed = GameConfig->AlienMoveSpeed;
		GetCapsuleComponent()->SetCapsuleSize(GameConfig->AlienCapsuleRadiusCm, GameConfig->AlienCapsuleHalfHeightCm);
		if (ArenaCollision)
		{
			ArenaCollision->GameConfig = GameConfig;
		}
	}
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

	UpdateHitFlash(DeltaSeconds);

	if (!bIsAlive)
	{
		return;
	}

	if (ArenaCollision)
	{
		ArenaCollision->PushApartNearbyAliens(this);
	}

	UpdateAI(DeltaSeconds);
}

void AAlienBot::UpdateHitFlash(float DeltaSeconds)
{
	if (HitFlashTimeRemaining <= 0.f)
	{
		HitFlashAlpha = 0.f;
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
	CombatState = EAlienCombatState::Chase;
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
	HitFlashTimeRemaining = 0.f;
	HitFlashAlpha = 0.f;
	if (bIsFlashing)
	{
		bIsFlashing = false;
		OnHitFlash.Broadcast(false);
	}
	SoftDespawn();
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

	const FVector ToPlayer = TargetPlayer->GetActorLocation() - GetActorLocation();
	FVector Desired = ToPlayer.GetSafeNormal2D();
	if (Desired.IsNearlyZero())
	{
		Desired = ToPlayer.GetSafeNormal();
	}

	// Simple obstacle steering: forward line trace; if blocked, add lateral offset (alternate sign).
	const FVector ProbeStart = GetActorLocation();
	const FVector ProbeEnd = ProbeStart + Desired * AlienBotPrivate::SteerProbeCm;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(AlienSteer), false, this);
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
		SteerHitScratch, ProbeStart, ProbeEnd, ECC_Visibility, Params);

	if (bBlocked && SteerHitScratch.GetActor() != TargetPlayer.Get())
	{
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Desired).GetSafeNormal();
		Desired = (Desired + Right * SteerSideSign * AlienBotPrivate::SteerLateralWeight).GetSafeNormal();
		SteerSideSign *= -1.f;
	}

	AddMovementInput(Desired, 1.f);
}

void AAlienBot::StrafeAndBurst(float DeltaSeconds)
{
	if (!TargetPlayer.IsValid())
	{
		return;
	}

	// Stop forward, strafe L/R (DESIGN)
	const FVector ToPlayer = (TargetPlayer->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, ToPlayer).GetSafeNormal();
	AddMovementInput(Right, StrafeSign);

	const int32 BurstCount = GameConfig ? GameConfig->AlienBurstRoundCount : 3;
	const float BurstInterval = GameConfig ? GameConfig->AlienBurstIntervalSeconds : 1.5f;
	const float IntraDelay = GameConfig ? GameConfig->AlienBurstIntraShotDelaySeconds : 0.09f;

	// Start a new burst when cooldown is done and no shots are queued.
	if (BurstCooldownRemaining <= 0.f && BurstShotsRemaining <= 0)
	{
		BurstShotsRemaining = BurstCount;
		BurstIntraShotRemaining = 0.f; // first shot fires this frame (after tick delay below)
		StrafeSign *= -1.f;
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
	const float Accuracy = GameConfig ? GameConfig->AlienAccuracy : 0.3f; // 30%
	if (FMath::FRand() > Accuracy)
	{
		return; // miss
	}
	const float Dmg = GameConfig ? GameConfig->AlienDamagePerHit : 10.f;
	UGameplayStatics::ApplyDamage(TargetPlayer.Get(), Dmg, GetController(), this, UDamageType::StaticClass());
	// TODO: optional tracer from alien muzzle
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

	const int32 BodyNeed = GameConfig ? GameConfig->AlienBodyHitsToKill : 3;
	const int32 HeadNeed = GameConfig ? GameConfig->AlienHeadshotsToKill : 2;
	if (HeadHitCount >= HeadNeed || BodyHitCount >= BodyNeed)
	{
		Die();
	}
	return Applied;
}

void AAlienBot::PlayHitFlash()
{
	// DESIGN 80ms white flash: bind mesh material emissive to HitFlashAlpha / OnHitFlash (Editor MID).
	const float Ms = GameConfig ? GameConfig->HitFlashDurationMs : 80.f;
	HitFlashTimeRemaining = Ms * 0.001f;
	HitFlashAlpha = 1.f;
	if (!bIsFlashing)
	{
		bIsFlashing = true;
		OnHitFlash.Broadcast(true);
	}
}

void AAlienBot::Die()
{
	bIsAlive = false;
	CombatState = EAlienCombatState::Dead;
	BurstShotsRemaining = 0;
	BurstIntraShotRemaining = 0.f;
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
	}
	// v1: collapse / hide / respawn — no ragdoll required
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);

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
	for (TActorIterator<AOfficeArena> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
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
