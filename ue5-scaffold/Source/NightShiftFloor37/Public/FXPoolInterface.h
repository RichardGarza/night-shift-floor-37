// Night Shift — Floor 37 | Object pools for tracers / muzzle lights (DESIGN performance)
// Concrete AFXPoolManager preallocates slots in BeginPlay; Acquire/Release cycle inactive slots.
// NO NewObject / SpawnActor in fire hot path after BeginPlay warmup.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameFramework/Actor.h"
#include "FXPoolInterface.generated.h"

class UGameConfig;
class UPointLightComponent;

UINTERFACE(MinimalAPI)
class UTracerPool : public UInterface
{
	GENERATED_BODY()
};

class NIGHTSHIFTFLOOR37_API ITracerPool
{
	GENERATED_BODY()
public:
	/** Activate a pooled tracer for DurationMs (DESIGN default 60). */
	virtual void ActivateTracer(const FVector& Start, const FVector& End, float DurationMs) = 0;
};

UINTERFACE(MinimalAPI)
class UMuzzleLightPool : public UInterface
{
	GENERATED_BODY()
};

class NIGHTSHIFTFLOOR37_API IMuzzleLightPool
{
	GENERATED_BODY()
public:
	/** Brief point light at muzzle (DESIGN). Pool size from UGameConfig::MuzzleLightPoolSize. */
	virtual void ActivateMuzzleLight(const FVector& WorldLocation, float DurationMs) = 0;
};

/** Lightweight placeholder tracer — hidden when inactive; no Niagara required for Phase 1. */
UCLASS()
class NIGHTSHIFTFLOOR37_API APooledTracerActor : public AActor
{
	GENERATED_BODY()
public:
	APooledTracerActor();

	void Activate(const FVector& Start, const FVector& End, float DurationSeconds);
	void Deactivate();
	bool IsAvailable() const { return !bActive; }

	UPROPERTY(BlueprintReadOnly, Category = "Pool")
	bool bActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Pool")
	FVector TracerStart = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Pool")
	FVector TracerEnd = FVector::ZeroVector;

	/** Thin bright box stretched from TracerStart to TracerEnd (engine cube). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pool")
	TObjectPtr<class UStaticMeshComponent> Mesh;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	float TimeRemaining = 0.f;
};

/**
 * World-placed (or auto-spawned) pool manager.
 * Preallocates TracerPoolSize / MuzzleLightPoolSize slots in BeginPlay.
 * Rifle finds via GetActorOfClass or soft ref.
 */
UCLASS()
class NIGHTSHIFTFLOOR37_API AFXPoolManager : public AActor, public ITracerPool, public IMuzzleLightPool
{
	GENERATED_BODY()

public:
	AFXPoolManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	TObjectPtr<UGameConfig> GameConfig;

	UPROPERTY(EditAnywhere, Category = "Pools")
	int32 TracerPoolSize = 32;

	UPROPERTY(EditAnywhere, Category = "Pools")
	int32 MuzzleLightPoolSize = 8;

	// ITracerPool / IMuzzleLightPool
	virtual void ActivateTracer(const FVector& Start, const FVector& End, float DurationMs) override;
	virtual void ActivateMuzzleLight(const FVector& WorldLocation, float DurationMs) override;

	UFUNCTION(BlueprintCallable, Category = "Pools")
	void WarmPools();

protected:
	struct FMuzzleSlot
	{
		UPointLightComponent* Light = nullptr;
		float TimeRemaining = 0.f;
		bool bActive = false;
	};

	UPROPERTY()
	TArray<TObjectPtr<APooledTracerActor>> TracerSlots;

	TArray<FMuzzleSlot> MuzzleSlots;

	int32 NextTracerIndex = 0;
	int32 NextMuzzleIndex = 0;

	void ApplyPoolSizesFromConfig();
};
