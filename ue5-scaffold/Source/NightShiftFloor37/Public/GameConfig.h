// Night Shift — Floor 37 | UGameConfig — ALL tunables (old CONFIG)
// Defaults MUST match DESIGN.md exactly. One data asset drives the match.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "GameConfig.generated.h"

/**
 * Single source of truth for every gameplay tunable.
 * Create a Data Asset of this class under Content/Data/ and assign it on GameMode / Character / Bots.
 * PERFORMANCE: read values into locals in hot paths; do not re-fetch UObject props every tick if avoidable.
 */
UCLASS(BlueprintType)
class NIGHTSHIFTFLOOR37_API UGameConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UGameConfig();

	// -------------------------------------------------------------------------
	// Player (DESIGN: 100 HP, regen 10/s after 5s, walk 6, sprint 9, jump 5,
	//         gravity 15, fall damage >6m, eye ~1.8m)
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player")
	float PlayerMaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player")
	float PlayerRegenPerSecond = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player")
	float PlayerRegenDelaySeconds = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player")
	float WalkSpeed = 600.f; // 6 m/s (UE cm)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player")
	float SprintSpeed = 900.f; // 9 m/s

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player")
	float JumpZVelocity = 500.f; // 5 m/s

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player")
	float GravityScale = 1.5306f; // ~15 m/s² vs default 980 cm/s² → 15/9.8 ≈ 1.53

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player")
	float FallDamageHeightMeters = 6.f;

	/** HP lost per metre fallen beyond FallDamageHeightMeters. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player")
	float FallDamagePerExcessMeter = 15.f;

	/** Degrees of camera turn per mouse count. Player-adjustable in the Esc menu; this is the default. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	float DefaultMouseSensitivity = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player")
	float CapsuleHalfHeightCm = 90.f; // ~1.8 m total height

	/** Forward probe distance for optional ledge mantle (cm). Phase 7 literal moved from character. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Movement")
	float MantleReachCm = 80.f;

	/** Max ledge height probe for mantle (cm). Phase 7 literal moved from character. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Movement")
	float MantleHeightCm = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player")
	float CapsuleRadiusCm = 42.f;

	// -------------------------------------------------------------------------
	// Rifle (DESIGN: 30 mag / 90 reserve, 600 RPM, hitscan, 25 body / 50 head,
	//         1.5s reload, small recoil)
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rifle")
	int32 MagSize = 30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rifle")
	int32 ReserveAmmo = 90;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rifle")
	float RoundsPerMinute = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rifle")
	float BodyDamage = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rifle")
	float HeadDamage = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rifle")
	float ReloadSeconds = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rifle")
	float RecoilPitchMaxDegrees = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rifle")
	float RecoilYawMaxDegrees = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rifle")
	float RecoilRecoverySpeed = 8.f;

	/**
	 * Fraction of each kick that is NOT recovered (0 = camera settles exactly back, DESIGN's
	 * "small kick that recovers"; 0.3 = spray climbs and the player pulls down). Open design decision.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rifle", meta = (ClampMin = "0", ClampMax = "1"))
	float RecoilPersistFraction = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rifle")
	float HitscanRangeMeters = 200.f;

	/** Soft-lock cone half-angle (degrees) when alien is in reticle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rifle")
	float SoftLockConeHalfAngle = 3.f;

	/** Soft-lock search radius. Kept well under HitscanRangeMeters: it is a per-shot overlap query. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rifle")
	float SoftLockRangeMeters = 30.f;

	// -------------------------------------------------------------------------
	// Aliens (DESIGN: 6 live, 4 m/s, ≤12m combat, 3-round burst / 1.5s,
	//         30% accuracy, 10 dmg, kill 3 body or 2 head, respawn 3s, 8 spawns)
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens")
	int32 MaxLiveAliens = 6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens")
	float AlienMoveSpeed = 400.f; // 4 m/s

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens")
	float AlienCombatRangeMeters = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens")
	int32 AlienBurstRoundCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens")
	float AlienBurstIntervalSeconds = 1.5f;

	/** Delay between shots inside a burst (seconds). DESIGN: ~0.08–0.1 so 3 shots are not dumped in one Tick. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens")
	float AlienBurstIntraShotDelaySeconds = 0.09f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens")
	float AlienAccuracy = 0.3f; // 30%

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens")
	float AlienDamagePerHit = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens")
	int32 AlienBodyHitsToKill = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens")
	int32 AlienHeadshotsToKill = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens")
	float AlienRespawnSeconds = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens")
	int32 AlienSpawnPointCount = 8;

	/** Head = top 25% of capsule (DESIGN). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens")
	float AlienHeadFraction = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens")
	float AlienCapsuleHalfHeightCm = 88.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens")
	float AlienCapsuleRadiusCm = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens")
	float AlienPushApartRadiusCm = 80.f;

	// -------------------------------------------------------------------------
	// Art | Phase 8 mesh swap (ModelFinder → Content/Imported). Soft refs; null = greybox.
	// Next sprint wires real assets; ResolveOrCreate defaults leave these unset.
	// -------------------------------------------------------------------------

	/**
	 * Quaternius Alien static mesh (replaces BasicShapes Cylinder).
	 * Default soft path: /Game/Imported/Aliens/SM_Alien — greybox if asset missing.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art|Phase8")
	TSoftObjectPtr<UStaticMesh> AlienBodyMesh;

	/**
	 * Optional head static mesh override for AAlienBot (replaces BasicShapes Sphere).
	 * Leave unset to keep greybox spheres.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art|Phase8")
	TSoftObjectPtr<UStaticMesh> AlienHeadMesh;

	/**
	 * Optional skeletal mesh for AAlienBot. When resolved, hides Body/Head static greybox
	 * and drives the character mesh. Prefer this once ModelFinder drops a skinned alien.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art|Phase8")
	TSoftObjectPtr<USkeletalMesh> AlienSkeletalMesh;

	/**
	 * Omie SM_Cubicle stamped at cover volume centers (non-colliding visual).
	 * Default soft path: /Game/Imported/Props/Office/SM_Cubicle. Collision stays on cover boxes.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art|Phase8")
	TSoftObjectPtr<UStaticMesh> CoverPropMesh;

	/**
	 * Sprint N — Omie SM_Desk near cubicle cover stamps (non-colliding dress).
	 * Default: /Game/Imported/Props/Office/SM_Desk. Soft miss → skip stamp.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art|Phase8")
	TSoftObjectPtr<UStaticMesh> DeskPropMesh;

	/**
	 * Sprint N — Omie SM_Chair near cubicle desks (non-colliding dress).
	 * Default: /Game/Imported/Props/Office/SM_Chair. Soft miss → skip stamp.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art|Phase8")
	TSoftObjectPtr<UStaticMesh> ChairPropMesh;

	/**
	 * Poly Haven mounted fluorescent (Phase 8). Import-only for Sprint G if unused in arena yet.
	 * Expected asset: /Game/Imported/Props/Lights/SM_MountedFluorescent
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art|Phase8")
	TSoftObjectPtr<UStaticMesh> FluorescentLightMesh;

	// -------------------------------------------------------------------------
	// Match (DESIGN: win at 25 kills, soft restart)
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match")
	int32 KillsToWin = 25;

	/**
	 * Sprint C — seconds after Click-to-play / SoftRestart→StartMatch before aliens fire or chase.
	 * Mid/late DESIGN numbers unchanged; grace is early-game only. Default 4s (range 3–5).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|EarlyGame")
	float SpawnGraceSeconds = 4.f;

	/** During grace, aliens stay Idle (no chase). If false, they may move but still cannot fire. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|EarlyGame")
	bool bSpawnGraceBlocksAlienAggro = true;

	/** Optional brief player damage immunity during spawn grace (alien fire already blocked). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|EarlyGame")
	bool bSpawnGracePlayerDamageImmune = true;

	/**
	 * On match start, refuse alien activations closer than this (meters) to the player;
	 * EnsureAlienPopulation already prefers farthest edge spawns — this is a hard floor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|EarlyGame")
	float MinStartSeparationMeters = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match")
	float MaxDeltaTimeClampSeconds = 0.05f; // treat spikes above ~50 ms as 50 ms

	// -------------------------------------------------------------------------
	// Arena (DESIGN: ~50x50 m atrium floor)
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena")
	float ArenaSizeMeters = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena")
	float AtriumTowerHeightMeters = 14.f;

	// -------------------------------------------------------------------------
	// Feedback (DESIGN: flash 80ms, tracer 60ms, muzzle point light, vignette)
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	float HitFlashDurationMs = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	float TracerDurationMs = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	float MuzzleFlashDurationMs = 40.f;

	/** Point-light intensity when a pooled muzzle flash activates (cd). Was literal 3000. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	float MuzzleLightIntensity = 3000.f;

	/** Crosshair hit-marker flash. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	float HitMarkerDurationMs = 120.f;

	/** Red damage overlay fade on the HUD. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	float DamageVignetteDurationMs = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	float CameraShakeScale = 0.35f;

	/** Object-pool sizes — avoid per-frame allocs (DESIGN performance rules). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pools")
	int32 TracerPoolSize = 32;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pools")
	int32 MuzzleLightPoolSize = 8;

	/** Helper: seconds between shots at Mag RPM. */
	UFUNCTION(BlueprintPure, Category = "Rifle")
	float GetSecondsPerShot() const { return RoundsPerMinute > 0.f ? 60.f / RoundsPerMinute : 0.1f; }

	/**
	 * Load /Game/Data/DA_GameConfig if Present; else NewObject with DESIGN defaults (PIE-safe).
	 * Pass Existing if already assigned on GameMode/Character.
	 */
	static UGameConfig* ResolveOrCreate(UObject* Outer, UGameConfig* Existing = nullptr);

	/** Assign Sprint G Content/Imported soft paths when soft refs are still null. */
	void EnsurePhase8DefaultSoftPaths();
};

