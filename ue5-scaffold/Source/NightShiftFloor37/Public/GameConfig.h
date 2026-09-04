// Night Shift — Floor 37 | UGameConfig — ALL tunables (old CONFIG)
// Defaults MUST match DESIGN.md exactly. One data asset drives the match.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
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

	/** Absolute gravity magnitude used when applying custom gravity (cm/s²). DESIGN: 15 m/s². */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player")
	float GravityMagnitude = 1500.f; // 15 m/s² in cm

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player")
	float FallDamageHeightMeters = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player")
	float EyeHeightMeters = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player")
	float CapsuleHalfHeightCm = 90.f; // ~1.8 m total height

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rifle")
	float HitscanRangeMeters = 200.f;

	/** Soft-lock cone half-angle (degrees) when alien is in reticle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rifle")
	float SoftLockConeHalfAngle = 3.f;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens")
	float AlienAccuracy = 0.3f; // 30%

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens")
	float AlienDamagePerHit = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens")
	int32 AlienBodyHitsToKill = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens")
	int32 AlienHeadshotsToKill = 2;

	/** Derived HP: bodyHits * bodyDmg equivalent for player rifle — bot tracks hit counts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens")
	float AlienMaxHealth = 75.f; // 3 * 25 body

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
	// Match (DESIGN: win at 25 kills, soft restart)
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match")
	int32 KillsToWin = 25;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	float DamageVignetteDurationMs = 200.f;

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
};
