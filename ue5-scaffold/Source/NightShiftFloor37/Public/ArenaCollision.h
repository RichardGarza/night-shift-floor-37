// Night Shift — Floor 37 | Extra traces, push-apart, fall damage
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/OverlapResult.h"
#include "ArenaCollision.generated.h"

class UGameConfig;
class AAlienBot;
class ANightShiftCharacter;

/**
 * Hand-rolled extras where CMC is not enough: cover heights, atrium drops, cheap bot push-apart.
 * No physics library beyond UE. PERFORMANCE: member overlap scratch; no per-frame heap growth.
 */
UCLASS(ClassGroup = (Arena), meta = (BlueprintSpawnableComponent))
class NIGHTSHIFTFLOOR37_API UArenaCollision : public UActorComponent
{
	GENERATED_BODY()

public:
	UArenaCollision();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	TObjectPtr<UGameConfig> GameConfig;

	/**
	 * Sphere push-apart vs nearby aliens (DESIGN: collide with each other).
	 * Overlaps into member scratch TArray (Reset, no heap growth after warm).
	 * Pushes SelfBot along XY by half the penetration vs AlienPushApartRadiusCm.
	 */
	UFUNCTION(BlueprintCallable, Category = "Collision")
	void PushApartNearbyAliens(AAlienBot* SelfBot);

	/**
	 * Fall damage if drop exceeds threshold meters (DESIGN: >6 m).
	 * Formula: Damage = (FallMeters - ThresholdMeters) * 15.f  (15 HP per excess meter).
	 * Threshold default = GameConfig::FallDamageHeightMeters (6). Applied via TakeDamage.
	 */
	UFUNCTION(BlueprintCallable, Category = "Collision")
	void ApplyFallDamageIfNeeded(ANightShiftCharacter* Character, float FallMeters, float ThresholdMeters);

	/** Downward line trace for atrium drops / ground distance. */
	UFUNCTION(BlueprintCallable, Category = "Collision")
	bool TraceGroundDistance(const FVector& From, float MaxDistanceCm, FHitResult& OutHit) const;

	/** Upward line trace for cover / mantel height checks. */
	UFUNCTION(BlueprintCallable, Category = "Collision")
	bool TraceCoverHeight(const FVector& From, float HeightCm, FHitResult& OutHit) const;

private:
	/** Reused by PushApartNearbyAliens — avoid allocating every AI tick. */
	TArray<FOverlapResult> OverlapScratch;
};
