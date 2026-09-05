#include "ArenaCollision.h"
#include "GameConfig.h"
#include "AlienBot.h"
#include "NightShiftCharacter.h"
#include "NightShiftFloor37.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "CollisionQueryParams.h"

UArenaCollision::UArenaCollision()
{
	PrimaryComponentTick.bCanEverTick = false;
	OverlapScratch.Reserve(16);
}

void UArenaCollision::BeginPlay()
{
	Super::BeginPlay();
	if (GameConfig)
	{
		return;
	}
	if (const ANightShiftCharacter* Player = Cast<ANightShiftCharacter>(GetOwner()))
	{
		GameConfig = Player->GameConfig;
	}
	else if (const AAlienBot* Bot = Cast<AAlienBot>(GetOwner()))
	{
		GameConfig = Bot->GameConfig;
	}
}

void UArenaCollision::PushApartNearbyAliens(AAlienBot* SelfBot)
{
	if (!SelfBot || !GetWorld())
	{
		return;
	}

	const float Radius = GameConfig ? GameConfig->AlienPushApartRadiusCm : 80.f;
	const FVector Origin = SelfBot->GetActorLocation();

	OverlapScratch.Reset();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(AlienPushApart), false, SelfBot);
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);
	GetWorld()->OverlapMultiByChannel(OverlapScratch, Origin, FQuat::Identity, ECC_Pawn, Sphere, Params);

	for (const FOverlapResult& O : OverlapScratch)
	{
		AAlienBot* Other = Cast<AAlienBot>(O.GetActor());
		if (!Other || Other == SelfBot || !Other->bIsAlive)
		{
			continue;
		}

		FVector Delta = Origin - Other->GetActorLocation();
		Delta.Z = 0.f; // XY-only push (DESIGN: cheap sphere push-apart)
		float Dist = Delta.Size();
		if (Dist < KINDA_SMALL_NUMBER)
		{
			// Coincident — pick a stable XY unit vector from actor IDs to avoid FRand heap noise.
			const uint32 Mix = GetTypeHash(SelfBot) ^ GetTypeHash(Other);
			Delta = FVector(
				(Mix & 1u) ? 1.f : -1.f,
				(Mix & 2u) ? 1.f : -1.f,
				0.f).GetSafeNormal();
			Dist = 0.f;
		}
		else
		{
			Delta /= Dist;
		}

		const float Push = (Radius - Dist) * 0.5f;
		if (Push > 0.f)
		{
			SelfBot->AddActorWorldOffset(Delta * Push, /*bSweep=*/true);
		}
	}
}

void UArenaCollision::ApplyFallDamageIfNeeded(ANightShiftCharacter* Character, float FallMeters, float ThresholdMeters)
{
	if (!Character || FallMeters <= ThresholdMeters)
	{
		return;
	}

	// -------------------------------------------------------------------------
	// Fall damage formula (DESIGN >6 m atrium drops):
	//   ExcessMeters = FallMeters - ThresholdMeters
	//   Damage       = ExcessMeters * 15.f     // 15 HP per excess meter
	//   Threshold    = GameConfig::FallDamageHeightMeters (default 6)
	// Applied through ANightShiftCharacter::TakeDamage so regen delay / HUD fire.
	// -------------------------------------------------------------------------
	const float Excess = FallMeters - ThresholdMeters;
	const float Damage = Excess * 15.f;
	UE_LOG(LogNightShift, Log, TEXT("Fall damage %.1f (fell %.1fm > %.1fm threshold)"),
		Damage, FallMeters, ThresholdMeters);
	Character->TakeDamage(Damage, FDamageEvent(), nullptr, Character);
}

bool UArenaCollision::TraceGroundDistance(const FVector& From, float MaxDistanceCm, FHitResult& OutHit) const
{
	UWorld* World = GetWorld();
	if (!World || MaxDistanceCm <= 0.f)
	{
		return false;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(GroundTrace), false, GetOwner());
	const FVector End = From - FVector(0.f, 0.f, MaxDistanceCm);
	return World->LineTraceSingleByChannel(OutHit, From, End, ECC_Visibility, Params);
}

bool UArenaCollision::TraceCoverHeight(const FVector& From, float HeightCm, FHitResult& OutHit) const
{
	UWorld* World = GetWorld();
	if (!World || HeightCm <= 0.f)
	{
		return false;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(CoverTrace), false, GetOwner());
	const FVector Start = From + FVector(0.f, 0.f, 10.f);
	const FVector End = From + FVector(0.f, 0.f, HeightCm);
	return World->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, Params);
}
