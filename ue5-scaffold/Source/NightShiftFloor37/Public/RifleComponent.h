// Night Shift — Floor 37 | Fire, reload, ammo, hitscan traces
// Phase 1: complete API for Enhanced Input wiring + hitscan + soft-lock.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/OverlapResult.h"
#include "RifleComponent.generated.h"

class UGameConfig;
class ANightShiftCharacter;
class AFXPoolManager;
class AAlienBot;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChanged, int32, Mag, int32, Reserve);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHitConfirmed, bool, bHeadshot);

/**
 * Hitscan rifle: 30/90, 600 RPM, 25 body / 50 head, 1.5s reload, small recoil.
 * Soft-lock biases aim toward closest alive alien in reticle cone (DESIGN).
 * Traces from camera/muzzle. Object-pool consumers for tracers/muzzle lights.
 * PERFORMANCE: reuse FHitResult / FCollisionQueryParams / SoftLockOverlaps; no per-shot heap allocs.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class NIGHTSHIFTFLOOR37_API URifleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URifleComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	TObjectPtr<UGameConfig> Config;

	/** Optional soft ref; resolved from world in BeginPlay if unset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	TObjectPtr<AFXPoolManager> FXPool;

	UPROPERTY(BlueprintReadOnly, Category = "Ammo")
	int32 MagAmmo = 30;

	UPROPERTY(BlueprintReadOnly, Category = "Ammo")
	int32 ReserveAmmo = 90;

	UPROPERTY(BlueprintReadOnly, Category = "Ammo")
	bool bIsReloading = false;

	UPROPERTY(BlueprintReadOnly, Category = "Fire")
	bool bWantsFire = false;

	UPROPERTY(BlueprintAssignable, Category = "Ammo")
	FOnAmmoChanged OnAmmoChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnHitConfirmed OnHitConfirmed;

	UFUNCTION(BlueprintCallable, Category = "Rifle")
	void InitializeFromConfig(UGameConfig* InConfig);

	UFUNCTION(BlueprintCallable, Category = "Rifle")
	void Fire();

	UFUNCTION(BlueprintCallable, Category = "Rifle")
	void StopFire();

	UFUNCTION(BlueprintCallable, Category = "Rifle")
	void Reload();

	UFUNCTION(BlueprintPure, Category = "Rifle")
	void GetAmmo(int32& OutMag, int32& OutReserve) const;

	UFUNCTION(BlueprintCallable, Category = "Rifle")
	void SoftResetAmmo();

	/** Hitscan from aim origin along (soft-lock biased) aim dir. Returns true if blocking hit. */
	UFUNCTION(BlueprintCallable, Category = "Rifle")
	bool Trace(FHitResult& OutHit, FVector& OutStart, FVector& OutEnd);

	UFUNCTION(BlueprintCallable, Category = "Rifle")
	void ApplyDamageToHit(const FHitResult& Hit);

protected:
	void TryFireShot();
	void FinishReload();
	void KickRecoil();
	void SpawnTracerFX(const FVector& Start, const FVector& End);
	void SpawnMuzzleFlashFX();
	bool IsHeadHit(const FHitResult& Hit) const;
	void ResolveFXPool();

	/** Soft-lock: closest alive alien in cone, or null. Biases Dir toward capsule center / head. */
	AAlienBot* FindSoftLockTarget(const FVector& Origin, const FVector& AimDir, float RangeCm) const;
	FVector GetSoftLockAimPoint(const AAlienBot* Bot, const FVector& Origin, const FVector& AimDir) const;

	float TimeSinceLastShot = 0.f;
	float ReloadTimeRemaining = 0.f;

	/** Cached query params — avoid rebuild/alloc each shot (hot path). */
	mutable FCollisionQueryParams CachedQueryParams;

	/** Reused soft-lock overlap buffer — no per-shot heap growth after reserve. */
	mutable TArray<FOverlapResult> SoftLockOverlaps;
};
