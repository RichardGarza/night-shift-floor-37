#include "AlienBot.h"
#include "GameConfig.h"
#include "NightShiftCharacter.h"
#include "ArenaCollision.h"
#include "ArenaGameMode.h"
#include "NightShiftFloor37.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

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
	}
	BurstCooldownRemaining = 0.f;
}

void AAlienBot::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float MaxDt = GameConfig ? GameConfig->MaxDeltaTimeClampSeconds : 0.05f;
	if (DeltaSeconds > MaxDt)
	{
		DeltaSeconds = MaxDt;
	}

	if (HitFlashTimeRemaining > 0.f)
	{
		HitFlashTimeRemaining -= DeltaSeconds * 1000.f; // stored as ms countdown helper — see PlayHitFlash
		if (HitFlashTimeRemaining < 0.f)
		{
			HitFlashTimeRemaining = 0.f;
			// TODO: restore material color from white flash
		}
	}

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

void AAlienBot::SetTarget(ANightShiftCharacter* InTarget)
{
	TargetPlayer = InTarget;
}

void AAlienBot::ActivateAtSpawn(const FTransform& SpawnTransform)
{
	SetActorTransform(SpawnTransform);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	bIsAlive = true;
	BodyHitCount = 0;
	HeadHitCount = 0;
	CombatState = EAlienCombatState::Chase;
	BurstCooldownRemaining = 0.f;
}

void AAlienBot::SoftDespawn()
{
	bIsAlive = false;
	CombatState = EAlienCombatState::Dead;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	GetCharacterMovement()->StopMovementImmediately();
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
		CombatState = EAlienCombatState::Chase;
		ChasePlayer(DeltaSeconds);
	}
}

void AAlienBot::ChasePlayer(float DeltaSeconds)
{
	(void)DeltaSeconds;
	if (!TargetPlayer.IsValid())
	{
		return;
	}
	// TODO: Navmesh MoveTo + simple steering. Waypoint graph if stairs fail.
	const FVector ToPlayer = TargetPlayer->GetActorLocation() - GetActorLocation();
	AddMovementInput(ToPlayer.GetSafeNormal(), 1.f);
}

void AAlienBot::StrafeAndBurst(float DeltaSeconds)
{
	(void)DeltaSeconds;
	if (!TargetPlayer.IsValid())
	{
		return;
	}
	// Stop forward, strafe L/R (DESIGN)
	const FVector ToPlayer = (TargetPlayer->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, ToPlayer).GetSafeNormal();
	AddMovementInput(Right, StrafeSign);

	if (BurstCooldownRemaining <= 0.f)
	{
		BurstShotsRemaining = GameConfig ? GameConfig->AlienBurstRoundCount : 3;
		BurstCooldownRemaining = GameConfig ? GameConfig->AlienBurstIntervalSeconds : 1.5f;
		StrafeSign *= -1.f;
	}

	if (BurstShotsRemaining > 0)
	{
		TryBurstShot();
		--BurstShotsRemaining;
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
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(AlienLOS), false, this);
	const FVector Start = GetActorLocation() + FVector(0, 0, 60);
	const FVector End = TargetPlayer->GetActorLocation() + FVector(0, 0, 60);
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	return !bBlocked || Hit.GetActor() == TargetPlayer.Get();
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
	// DESIGN: flash white 80 ms
	HitFlashTimeRemaining = GameConfig ? GameConfig->HitFlashDurationMs : 80.f;
	// TODO: set emissive/white overlay on mesh material
}

void AAlienBot::Die()
{
	bIsAlive = false;
	CombatState = EAlienCombatState::Dead;
	GetCharacterMovement()->StopMovementImmediately();
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
	GetWorldTimerManager().SetTimer(RespawnTimerHandle, [this]()
	{
		// TODO: Ask GameMode/Arena for farthest spawn; ActivateAtSpawn
		UE_LOG(LogNightShift, Log, TEXT("AlienBot respawn timer fired — wire to arena spawn."));
	}, Delay, false);
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

