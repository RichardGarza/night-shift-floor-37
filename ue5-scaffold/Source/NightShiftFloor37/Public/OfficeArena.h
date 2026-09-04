// Night Shift — Floor 37 | Bounds, spawn points, cover volumes
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OfficeArena.generated.h"

class UBoxComponent;
class UGameConfig;

/**
 * Persistent arena actor: floor bounds (~50x50), 8 alien spawn points, cover volumes.
 * Place once in the level. Invisible ceiling / bounds clamp so nobody leaves the floor.
 *
 * Layout zones (DESIGN — around atrium center, all walkable on top):
 *   - Atrium center: 3-story stair/catwalk tower (~AtriumTowerHeightCm)
 *   - Cubicle maze (low cover) — default CoverVolumes approximate this
 *   - 6 server racks / IT cages
 *   - Cable tray / pipe run at 1.5 m height
 *   - Raised conference pad / broken glass boardroom
 *   - Collapsed drywall berm + planters
 *   - 4 resin / egg barrel clusters (low cover)
 *   - Perimeter: exterior glass + low planters; BoundsVolume + CeilingClamp
 *
 * Procedural defaults let GameMode/AlienBot run without Editor art; place real
 * SpawnPointActors and cover boxes for shipping levels.
 */
UCLASS()
class NIGHTSHIFTFLOOR37_API AOfficeArena : public AActor
{
	GENERATED_BODY()

public:
	AOfficeArena();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	TObjectPtr<UGameConfig> GameConfig;

	/** World-space alien spawn transforms. DESIGN expects 8 fixed edge points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawns")
	TArray<FTransform> AlienSpawnPoints;

	/** Optional named markers already placed in the level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawns")
	TArray<TObjectPtr<AActor>> SpawnPointActors;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bounds")
	TObjectPtr<UBoxComponent> BoundsVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bounds")
	TObjectPtr<UBoxComponent> CeilingClamp;

	/** Low-cover volumes (cubicles, resin barrels, etc.) — for AI / soft traces. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	TArray<TObjectPtr<UBoxComponent>> CoverVolumes;

	/**
	 * Half-extent of walkable floor in cm (XY). Synced from GameConfig::ArenaSizeMeters on BeginPlay.
	 * Default 2500 → 50 m full span.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Layout")
	float ArenaHalfExtentCm = 2500.f;

	/**
	 * Atrium tower height in cm. Synced from GameConfig::AtriumTowerHeightMeters on BeginPlay.
	 * Default 1400 → 14 m (DESIGN).
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Layout")
	float AtriumTowerHeightCm = 1400.f;

	/** Pick spawn farthest from WorldLocation (DESIGN: on spawn, farthest from player). */
	UFUNCTION(BlueprintCallable, Category = "Spawns")
	FTransform GetFarthestSpawnFrom(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintCallable, Category = "Spawns")
	void GatherSpawnPointsFromActors();

	/**
	 * If AlienSpawnPoints is empty after GatherSpawnPointsFromActors, generate edge spawns
	 * on the walkable square (±~2300 cm from center). Count matches GameConfig::AlienSpawnPointCount (8).
	 * Conceptual labels: stairwells, loading dock, elevator bank, service corridor (mid-edge + corners).
	 */
	UFUNCTION(BlueprintCallable, Category = "Spawns")
	void EnsureDefaultSpawns();

	/** Soft-reset helper: re-gather markers then fill procedural defaults if still short. */
	UFUNCTION(BlueprintCallable, Category = "Spawns")
	void RefreshSpawnGather();

	UFUNCTION(BlueprintPure, Category = "Bounds")
	bool IsInsideBounds(const FVector& WorldLocation) const;

	/** AABB/OBB clamp into BoundsVolume; also clamps Z below CeilingClamp bottom. */
	UFUNCTION(BlueprintCallable, Category = "Bounds")
	FVector ClampToBounds(const FVector& WorldLocation) const;

	/** Sets actor location to ClampToBounds if outside (GameMode tick / character hook). */
	UFUNCTION(BlueprintCallable, Category = "Bounds")
	void EnforceBoundsOnActor(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "Cover")
	void RegisterCoverVolume(UBoxComponent* Volume);

	UFUNCTION(BlueprintPure, Category = "Cover")
	bool IsPointInCover(const FVector& WorldLocation) const;

	/** Nearest cover sample point (volume center). Returns false if no volumes. */
	UFUNCTION(BlueprintCallable, Category = "Cover")
	bool GetNearestCoverPoint(const FVector& WorldLocation, FVector& OutCoverPoint) const;

protected:
	/** Prototype cubicle-ish boxes around atrium — no BP required. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<UBoxComponent> DefaultCover_CubicleN;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<UBoxComponent> DefaultCover_CubicleE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<UBoxComponent> DefaultCover_CubicleS;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<UBoxComponent> DefaultCover_CubicleW;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<UBoxComponent> DefaultCover_ResinNE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<UBoxComponent> DefaultCover_ResinSW;

	void SyncLayoutFromConfig();
	void SetupDefaultCoverVolume(UBoxComponent* Box, const FVector& RelativeLocation, const FVector& Extent);
};
