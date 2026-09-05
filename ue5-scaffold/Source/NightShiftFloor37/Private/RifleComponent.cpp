#include "RifleComponent.h"
#include "GameConfig.h"
#include "NightShiftCharacter.h"
#include "AlienBot.h"
#include "ArenaGameMode.h"
#include "FXPoolInterface.h"
#include "NightShiftFloor37.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"

URifleComponent::URifleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SoftLockOverlaps.Reserve(16);
}

void URifleComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!Config)
	{
		if (const ANightShiftCharacter* Char = Cast<ANightShiftCharacter>(GetOwner()))
		{
			Config = Char->GameConfig;
		}
	}
	if (Config)
	{
		InitializeFromConfig(Config);
	}
	CachedQueryParams = FCollisionQueryParams(SCENE_QUERY_STAT(RifleHitscan), false, GetOwner());
	ResolveFXPool();
}

void URifleComponent::ResolveFXPool()
{
	if (FXPool || !GetWorld())
	{
		return;
	}
	FXPool = Cast<AFXPoolManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AFXPoolManager::StaticClass()));
}

void URifleComponent::InitializeFromConfig(UGameConfig* InConfig)
{
	Config = InConfig;
	if (!Config)
	{
		return;
	}
	MagAmmo = Config->MagSize;
	ReserveAmmo = Config->ReserveAmmo;
	OnAmmoChanged.Broadcast(MagAmmo, ReserveAmmo);
}

void URifleComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float MaxDt = Config ? Config->MaxDeltaTimeClampSeconds : 0.05f;
	if (DeltaTime > MaxDt)
	{
		DeltaTime = MaxDt;
	}

	// Esc pause freezes firing and reload timers.
	if (const AArenaGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AArenaGameMode>() : nullptr)
	{
		if (GM->IsMatchPaused())
		{
			return;
		}
	}

	TimeSinceLastShot += DeltaTime;

	if (bIsReloading)
	{
		ReloadTimeRemaining -= DeltaTime;
		if (ReloadTimeRemaining <= 0.f)
		{
			FinishReload();
		}
		return;
	}

	if (bWantsFire)
	{
		TryFireShot();
	}
}

void URifleComponent::Fire()
{
	bWantsFire = true;
	TryFireShot();
}

void URifleComponent::StopFire()
{
	bWantsFire = false;
}

void URifleComponent::TryFireShot()
{
	if (bIsReloading || MagAmmo <= 0)
	{
		if (MagAmmo <= 0 && !bIsReloading)
		{
			Reload();
		}
		return;
	}

	const float Interval = Config ? Config->GetSecondsPerShot() : (60.f / 600.f);
	if (TimeSinceLastShot < Interval)
	{
		return;
	}

	TimeSinceLastShot = 0.f;
	--MagAmmo;
	OnAmmoChanged.Broadcast(MagAmmo, ReserveAmmo);

	FHitResult Hit;
	FVector Start, End;
	const bool bHit = Trace(Hit, Start, End);

	SpawnTracerFX(Start, bHit ? Hit.ImpactPoint : End);
	SpawnMuzzleFlashFX();
	KickRecoil();

	if (bHit)
	{
		ApplyDamageToHit(Hit);
	}
}

AAlienBot* URifleComponent::FindSoftLockTarget(const FVector& Origin, const FVector& AimDir, float RangeCm) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const float HalfAngleDeg = Config ? Config->SoftLockConeHalfAngle : 3.f;
	const float CosHalf = FMath::Cos(FMath::DegreesToRadians(HalfAngleDeg));

	SoftLockOverlaps.Reset();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RifleSoftLock), false, GetOwner());
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(RangeCm);
	World->OverlapMultiByChannel(SoftLockOverlaps, Origin, FQuat::Identity, ECC_Pawn, Sphere, Params);

	AAlienBot* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (const FOverlapResult& O : SoftLockOverlaps)
	{
		AAlienBot* Bot = Cast<AAlienBot>(O.GetActor());
		if (!Bot || !Bot->bIsAlive)
		{
			continue;
		}

		const FVector ToCenter = Bot->GetActorLocation() - Origin;
		const float DistSq = ToCenter.SizeSquared();
		if (DistSq > RangeCm * RangeCm || DistSq < KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const FVector ToNorm = ToCenter.GetSafeNormal();
		if (FVector::DotProduct(AimDir, ToNorm) < CosHalf)
		{
			continue;
		}

		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Bot;
		}
	}

	return Best;
}

FVector URifleComponent::GetSoftLockAimPoint(const AAlienBot* Bot, const FVector& Origin, const FVector& AimDir) const
{
	if (!Bot || !Bot->GetCapsuleComponent())
	{
		return Origin + AimDir;
	}

	const float HalfH = Bot->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Center = Bot->GetActorLocation();
	const float HeadFrac = Bot->GameConfig ? Bot->GameConfig->AlienHeadFraction
		: (Config ? Config->AlienHeadFraction : 0.25f);
	const FVector Head = Center + FVector(0.f, 0.f, HalfH * (1.f - HeadFrac));

	const FVector ToCenter = (Center - Origin).GetSafeNormal();
	const FVector ToHead = (Head - Origin).GetSafeNormal();
	const float DotCenter = FVector::DotProduct(AimDir, ToCenter);
	const float DotHead = FVector::DotProduct(AimDir, ToHead);

	return (DotHead >= DotCenter) ? Head : Center;
}

bool URifleComponent::Trace(FHitResult& OutHit, FVector& OutStart, FVector& OutEnd)
{
	const ANightShiftCharacter* Char = Cast<ANightShiftCharacter>(GetOwner());
	OutStart = Char ? Char->GetAimOrigin() : GetOwner()->GetActorLocation();
	FVector Dir = Char ? Char->GetAimDirection() : GetOwner()->GetActorForwardVector();
	const float RangeCm = (Config ? Config->HitscanRangeMeters : 200.f) * 100.f;

	const float SoftLockCm = FMath::Min(RangeCm, (Config ? Config->SoftLockRangeMeters : 30.f) * 100.f);
	if (AAlienBot* SoftTarget = FindSoftLockTarget(OutStart, Dir, SoftLockCm))
	{
		const FVector AimPoint = GetSoftLockAimPoint(SoftTarget, OutStart, Dir);
		Dir = (AimPoint - OutStart).GetSafeNormal();
	}

	OutEnd = OutStart + Dir * RangeCm;

	CachedQueryParams.ClearIgnoredSourceObjects();
	CachedQueryParams.AddIgnoredActor(GetOwner());

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	return World->LineTraceSingleByChannel(OutHit, OutStart, OutEnd, ECC_Visibility, CachedQueryParams);
}

void URifleComponent::ApplyDamageToHit(const FHitResult& Hit)
{
	AActor* HitActor = Hit.GetActor();
	if (!HitActor)
	{
		return;
	}

	const bool bHead = IsHeadHit(Hit);
	const float Damage = bHead
		? (Config ? Config->HeadDamage : 50.f)
		: (Config ? Config->BodyDamage : 25.f);

	UGameplayStatics::ApplyPointDamage(HitActor, Damage, Hit.ImpactNormal, Hit,
		GetOwner()->GetInstigatorController(), GetOwner(), UDamageType::StaticClass());

	OnHitConfirmed.Broadcast(bHead);
}

bool URifleComponent::IsHeadHit(const FHitResult& Hit) const
{
	if (const AAlienBot* Bot = Cast<AAlienBot>(Hit.GetActor()))
	{
		return Bot->IsHeadBone(Hit.BoneName) || Bot->IsLocationOnHead(Hit.ImpactPoint);
	}
	return Hit.BoneName == FName(TEXT("head")) || Hit.BoneName == FName(TEXT("Head"));
}

void URifleComponent::Reload()
{
	if (bIsReloading || MagAmmo == (Config ? Config->MagSize : 30) || ReserveAmmo <= 0)
	{
		return;
	}
	bIsReloading = true;
	ReloadTimeRemaining = Config ? Config->ReloadSeconds : 1.5f;
	UE_LOG(LogNightShift, Verbose, TEXT("Reload started (%.1fs)"), ReloadTimeRemaining);
}

void URifleComponent::FinishReload()
{
	bIsReloading = false;
	const int32 MagSize = Config ? Config->MagSize : 30;
	const int32 Need = MagSize - MagAmmo;
	const int32 Take = FMath::Min(Need, ReserveAmmo);
	MagAmmo += Take;
	ReserveAmmo -= Take;
	OnAmmoChanged.Broadcast(MagAmmo, ReserveAmmo);
}

void URifleComponent::GetAmmo(int32& OutMag, int32& OutReserve) const
{
	OutMag = MagAmmo;
	OutReserve = ReserveAmmo;
}

void URifleComponent::SoftResetAmmo()
{
	MagAmmo = Config ? Config->MagSize : 30;
	ReserveAmmo = Config ? Config->ReserveAmmo : 90;
	bIsReloading = false;
	bWantsFire = false;
	ReloadTimeRemaining = 0.f;
	OnAmmoChanged.Broadcast(MagAmmo, ReserveAmmo);
}

void URifleComponent::KickRecoil()
{
	ANightShiftCharacter* Char = Cast<ANightShiftCharacter>(GetOwner());
	if (!Char)
	{
		return;
	}
	const float PitchMax = Config ? Config->RecoilPitchMaxDegrees : 1.2f;
	const float YawMax = Config ? Config->RecoilYawMaxDegrees : 0.4f;
	const float Pitch = FMath::FRandRange(0.2f, PitchMax);
	const float Yaw = FMath::FRandRange(-YawMax, YawMax);
	Char->AddRecoilKick(Pitch, Yaw);
}

void URifleComponent::SpawnTracerFX(const FVector& Start, const FVector& End)
{
	if (!FXPool)
	{
		ResolveFXPool();
	}
	if (FXPool)
	{
		const float Ms = Config ? Config->TracerDurationMs : 60.f;
		FXPool->ActivateTracer(Start, End, Ms);
	}
}

void URifleComponent::SpawnMuzzleFlashFX()
{
	if (!FXPool)
	{
		ResolveFXPool();
	}
	if (!FXPool)
	{
		return;
	}

	FVector MuzzleLoc = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
	if (const ANightShiftCharacter* Char = Cast<ANightShiftCharacter>(GetOwner()))
	{
		MuzzleLoc = Char->GetAimOrigin() + Char->GetAimDirection() * 40.f;
	}
	const float Ms = Config ? Config->MuzzleFlashDurationMs : 40.f;
	FXPool->ActivateMuzzleLight(MuzzleLoc, Ms);
}
