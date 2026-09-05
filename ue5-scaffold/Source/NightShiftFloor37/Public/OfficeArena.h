// Night Shift — Floor 37 | Bounds, spawn points, cover volumes
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OfficeArena.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UDirectionalLightComponent;
class USkyLightComponent;
class USkyAtmosphereComponent;
class UExponentialHeightFogComponent;
class UPointLightComponent;
class UGameConfig;

/**
 * Named alien edge spawn (DESIGN: 8 fixed points — stairwells, loading dock,
 * elevator bank, service corridor). Ids:
 *   Stairwell_N, Stairwell_S, LoadingDock_E, ElevatorBank_W,
 *   ServiceCorridor_NE, ServiceCorridor_NW, ServiceCorridor_SE, ServiceCorridor_SW
 */
USTRUCT(BlueprintType)
struct FOfficeArenaSpawnPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawns")
	FName Id = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawns")
	FTransform Transform = FTransform::Identity;
};

/**
 * Persistent arena actor: floor bounds (~50x50), 8 alien spawn points, cover volumes.
 * Place once in the level. Invisible ceiling / bounds clamp so nobody leaves the floor.
 *
 * Layout zones (DESIGN — around atrium center, all walkable on top):
 *   - Atrium center: 3-story stair/catwalk tower (~AtriumTowerHeightCm)
 *   - Cubicle maze (low cover) — default CoverVolumes approximate this
 *   - 6 server racks / IT cages (two stacked, one angled) — RackStack_A/B + RackAngled proxies
 *   - Cable tray / pipe run at 1.5 m height
 *   - Raised conference pad / broken glass boardroom
 *   - Collapsed drywall berm + planters
 *   - 4 resin / egg barrel clusters (NE/NW/SE/SW low cover)
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

	/**
	 * Labeled edge spawn defs (preferred source). Keep in sync with AlienSpawnPoints
	 * so legacy GameMode/AlienBot callers that only read transforms keep working.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawns")
	TArray<FOfficeArenaSpawnPoint> AlienSpawnPointData;

	/** World-space alien spawn transforms. DESIGN expects 8 fixed edge points. Synced from AlienSpawnPointData. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawns")
	TArray<FTransform> AlienSpawnPoints;

	/** Optional named markers already placed in the level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawns")
	TArray<TObjectPtr<AActor>> SpawnPointActors;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bounds")
	TObjectPtr<UBoxComponent> BoundsVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bounds")
	TObjectPtr<UBoxComponent> CeilingClamp;

	/** Low-cover volumes (cubicles, resin barrels, racks, etc.) — for AI / soft traces. */
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

	/** Max search radius (cm) for FindCoverAwayFromThreat. 0 = unlimited (all registered cover). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	float CoverSearchRadiusCm = 2500.f;

	// ----- Greybox (code-built floor, walls, atrium tower, cover blocks, lighting) -----

	/** Build playable greybox geometry + lighting from engine basic shapes. Turn off once a real level exists. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greybox")
	bool bBuildGreybox = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Greybox")
	TArray<TObjectPtr<UStaticMeshComponent>> GreyboxMeshes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Greybox")
	TObjectPtr<UDirectionalLightComponent> SunLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Greybox")
	TObjectPtr<USkyLightComponent> SkyLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Greybox")
	TObjectPtr<USkyAtmosphereComponent> SkyAtmosphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Greybox")
	TObjectPtr<UExponentialHeightFogComponent> Fog;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Greybox")
	TArray<TObjectPtr<UPointLightComponent>> PracticalLights;

	/** Pick spawn farthest from WorldLocation (DESIGN: on spawn, farthest from player). Prefers AlienSpawnPointData. */
	UFUNCTION(BlueprintCallable, Category = "Spawns")
	FTransform GetFarthestSpawnFrom(const FVector& WorldLocation) const;

	/**
	 * Like GetFarthestSpawnFrom, but skips indices in ExcludeIndices so a batch of activations
	 * spreads across distinct points (otherwise every bot lands on the same corner). Falls back to
	 * the unfiltered pick when every index is excluded. OutIndex = chosen index, -1 if no spawns.
	 */
	FTransform GetFarthestUnusedSpawnFrom(const FVector& WorldLocation, const TArray<int32>& ExcludeIndices, int32& OutIndex) const;

	UFUNCTION(BlueprintPure, Category = "Spawns")
	int32 GetSpawnPointCount() const;

	/** Lookup by Id in AlienSpawnPointData (falls back to scanning labels in data only). */
	UFUNCTION(BlueprintCallable, Category = "Spawns")
	bool GetSpawnPointById(FName Id, FTransform& OutTransform) const;

	UFUNCTION(BlueprintCallable, Category = "Spawns")
	void GatherSpawnPointsFromActors();

	/**
	 * If spawn lists are empty/short after GatherSpawnPointsFromActors, generate 8 labeled
	 * edge spawns on the walkable square (±ArenaHalfExtentCm − inset). Fills AlienSpawnPointData
	 * and mirrors transforms into AlienSpawnPoints.
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

	UFUNCTION(BlueprintCallable, Category = "Cover")
	void UnregisterCoverVolume(UBoxComponent* Volume);

	UFUNCTION(BlueprintPure, Category = "Cover")
	int32 GetCoverVolumeCount() const;

	UFUNCTION(BlueprintPure, Category = "Cover")
	bool IsPointInCover(const FVector& WorldLocation) const;

	/** Nearest cover sample point (volume center). Returns false if no volumes. */
	UFUNCTION(BlueprintCallable, Category = "Cover")
	bool GetNearestCoverPoint(const FVector& WorldLocation, FVector& OutCoverPoint) const;

	/**
	 * Pick registered cover center farthest from ThreatLocation among volumes within
	 * CoverSearchRadiusCm of From (or all if radius <= 0). Efficient max-dist-from-threat
	 * among nearby candidates — good for "break contact / get behind something."
	 * Returns false if no cover volumes (or none in range).
	 */
	UFUNCTION(BlueprintCallable, Category = "Cover")
	bool FindCoverAwayFromThreat(const FVector& From, const FVector& ThreatLocation, FVector& OutCoverPoint) const;

	/**
	 * True if the world-space segment Start→End intersects any registered cover box
	 * (line vs OBB via each UBoxComponent's local AABB).
	 */
	UFUNCTION(BlueprintPure, Category = "Cover")
	bool DoesLineHitCover(const FVector& Start, const FVector& End) const;

protected:
	void BuildGreybox();
	void BuildGreyboxLighting();
	/** Create the MIDs that colour the greybox (BeginPlay; MIDs cannot exist in the constructor). */
	void ApplyGreyboxColors();
	UStaticMeshComponent* AddGreyboxBox(const FString& Name, const FVector& Center, const FVector& Size, const FRotator& Rot, const FLinearColor& Color);
	/** Sloped box whose top surface runs from SurfaceStart to SurfaceEnd (world-relative cm). */
	UStaticMeshComponent* AddGreyboxRamp(const FString& Name, const FVector& SurfaceStart, const FVector& SurfaceEnd, float Width, const FLinearColor& Color);
	TArray<FLinearColor> GreyboxColors;
	UPROPERTY()
	TObjectPtr<UStaticMesh> GreyboxCubeMesh;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> GreyboxMaterial;

	/** Prototype cubicle-ish boxes around atrium — no BP required (~800 cm ring). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<UBoxComponent> DefaultCover_CubicleN;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<UBoxComponent> DefaultCover_CubicleE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<UBoxComponent> DefaultCover_CubicleS;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<UBoxComponent> DefaultCover_CubicleW;

	/** Four resin / egg barrel clusters (DESIGN: NE/NW/SE/SW). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<UBoxComponent> DefaultCover_ResinNE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<UBoxComponent> DefaultCover_ResinNW;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<UBoxComponent> DefaultCover_ResinSE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<UBoxComponent> DefaultCover_ResinSW;

	/**
	 * Server-rack / IT-cage proxies for DESIGN's 6 racks (two stacked, one angled).
	 * Query boxes approximate stacks + angled unit — replace with real meshes in Editor.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<UBoxComponent> DefaultCover_RackStack_A;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<UBoxComponent> DefaultCover_RackStack_B;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<UBoxComponent> DefaultCover_RackAngled;

	void SyncLayoutFromConfig();
	void SetupDefaultCoverVolume(UBoxComponent* Box, const FVector& RelativeLocation, const FVector& Extent);
	void SetupDefaultCoverVolumeRotated(UBoxComponent* Box, const FVector& RelativeLocation, const FVector& Extent, const FRotator& RelativeRotation);
	void SyncSpawnTransformsFromData();
};
