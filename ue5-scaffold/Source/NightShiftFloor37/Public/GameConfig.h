// Night Shift — Floor 37 | UGameConfig — ALL tunables (old CONFIG)
// Defaults MUST match DESIGN.md exactly. One data asset drives the match.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "GameConfig.generated.h"

/** Sprint W — rifle-carry animation set for the player mannequin (code-driven, no AnimBP). */
USTRUCT(BlueprintType)
struct FNightShiftPlayerAnimSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim") TSoftObjectPtr<UAnimSequence> Idle;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim") TSoftObjectPtr<UAnimSequence> WalkFwd;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim") TSoftObjectPtr<UAnimSequence> WalkBwd;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim") TSoftObjectPtr<UAnimSequence> WalkLeft;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim") TSoftObjectPtr<UAnimSequence> WalkRight;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim") TSoftObjectPtr<UAnimSequence> JogFwd;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim") TSoftObjectPtr<UAnimSequence> JogBwd;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim") TSoftObjectPtr<UAnimSequence> JogLeft;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim") TSoftObjectPtr<UAnimSequence> JogRight;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim") TSoftObjectPtr<UAnimSequence> JumpStart;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim") TSoftObjectPtr<UAnimSequence> FallLoop;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim") TSoftObjectPtr<UAnimSequence> Land;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim") TSoftObjectPtr<UAnimSequence> Reload;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim") TSoftObjectPtr<UAnimSequence> Death;
};

/** Sprint X — animation set for the skeletal alien (Quaternius Alien.fbx takes). */
USTRUCT(BlueprintType)
struct FNightShiftAlienAnimSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim") TSoftObjectPtr<UAnimSequence> Idle;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim") TSoftObjectPtr<UAnimSequence> Walk;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim") TSoftObjectPtr<UAnimSequence> Run;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim") TSoftObjectPtr<UAnimSequence> Attack;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim") TSoftObjectPtr<UAnimSequence> HitReact;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim") TSoftObjectPtr<UAnimSequence> Death;
};

/** Resolved (loaded) mirror of FNightShiftPlayerAnimSet. Transient. */
USTRUCT(BlueprintType)
struct FNightShiftPlayerAnimCache
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Anim") TObjectPtr<UAnimSequence> Idle;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Anim") TObjectPtr<UAnimSequence> WalkFwd;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Anim") TObjectPtr<UAnimSequence> WalkBwd;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Anim") TObjectPtr<UAnimSequence> WalkLeft;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Anim") TObjectPtr<UAnimSequence> WalkRight;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Anim") TObjectPtr<UAnimSequence> JogFwd;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Anim") TObjectPtr<UAnimSequence> JogBwd;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Anim") TObjectPtr<UAnimSequence> JogLeft;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Anim") TObjectPtr<UAnimSequence> JogRight;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Anim") TObjectPtr<UAnimSequence> JumpStart;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Anim") TObjectPtr<UAnimSequence> FallLoop;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Anim") TObjectPtr<UAnimSequence> Land;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Anim") TObjectPtr<UAnimSequence> Reload;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Anim") TObjectPtr<UAnimSequence> Death;
};

/** Resolved (loaded) mirror of FNightShiftAlienAnimSet. Transient. */
USTRUCT(BlueprintType)
struct FNightShiftAlienAnimCache
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Anim") TObjectPtr<UAnimSequence> Idle;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Anim") TObjectPtr<UAnimSequence> Walk;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Anim") TObjectPtr<UAnimSequence> Run;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Anim") TObjectPtr<UAnimSequence> Attack;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Anim") TObjectPtr<UAnimSequence> HitReact;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Anim") TObjectPtr<UAnimSequence> Death;
};

/** Sprint Z — alien variants introduced by the wave progression. */
UENUM(BlueprintType)
enum class EAlienVariant : uint8
{
	Grunt,
	Brute,
	Stalker
};

/** Sprint Z — how a variant differs from the Grunt (DESIGN numbers). Scale/speed/HP/fire/glow. */
USTRUCT(BlueprintType)
struct FAlienVariantTuning
{
	GENERATED_BODY()

	/** First wave this variant may appear on. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant") int32 FromWave = 3;
	/** Max fraction of the live target this variant may occupy (at least one once unlocked). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant") float MaxShareOfLive = 0.34f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant") float ScaleMul = 1.35f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant") float SpeedMul = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant") int32 BodyHitsToKill = 6;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant") int32 HeadshotsToKill = 3;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant") float BurstIntervalMul = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant") float AccuracyBonus = 0.f;
	/** Always-on point light so the variant reads at a glance (the atlas material ignores tints). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant") FLinearColor GlowColor = FLinearColor(1.f, 0.15f, 0.05f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant") float GlowIntensity = 400.f;
};

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

	/**
	 * Sprint Y — holding fire drops the player from sprint to walk speed (Richard: shooting felt bad
	 * while running — 9 m/s + jog clip at 1.9× bounced the rifle). Sprint resumes once the trigger
	 * has been released for SprintResumeAfterFireSeconds while Shift is still held.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rifle")
	bool bFireCancelsSprint = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rifle")
	float SprintResumeAfterFireSeconds = 0.4f;

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

	/** Sprint Y — yaw turn rate (deg/s) used to face the player while in combat range. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens|Behaviour")
	float AlienFaceTargetTurnRateDegPerSec = 540.f;

	/** Sprint Y — a burst may only start once the alien faces the player within this many degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens|Behaviour")
	float AlienFireFacingToleranceDegrees = 25.f;

	/** Sprint Y — stop strafing while the attack clip / burst plays (strafe between bursts only). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens|Behaviour")
	bool bAlienPlantsDuringBurst = true;

	/** Sprint Y — once a steer side is picked around an obstacle, hold it this long (no per-frame flip). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens|Behaviour")
	float AlienSteerCommitSeconds = 0.6f;

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
	 * Sprint X — uniform scale applied to AlienSkeletalMesh (Quaternius "Big" alien is ~3.5 m
	 * at native scale; 0.55 → ~1.9 m). Capsule is refit to the scaled mesh bounds.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art|Phase8")
	float AlienMeshScale = 0.55f;

	/** Sprint X — mesh yaw so the alien faces +X (FBX/Blender exports usually need -90). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art|Phase8")
	float AlienMeshYawDegrees = -90.f;

	/** Sprint X — alien animation takes. Default: /Game/Imported/Aliens/Skel/SK_AlienCharacterArmature_*. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art|Phase8")
	FNightShiftAlienAnimSet AlienAnims;

	/**
	 * Sprint W — player skeletal mesh (UE template Manny). When resolved, hides the greybox
	 * cylinder and drives ACharacter::GetMesh(). Default: /Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art|Phase8")
	TSoftObjectPtr<USkeletalMesh> PlayerSkeletalMesh;

	/** Sprint W — mesh yaw so the mannequin faces +X (template convention -90). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art|Phase8")
	float PlayerMeshYawDegrees = -90.f;

	/**
	 * Sprint W — rifle static mesh attached to the player mesh socket RifleSocketName.
	 * Default: /Game/Weapons/Rifle/Meshes/SM_Rifle. Null / soft-miss → no rifle prop (hitscan still works).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art|Phase8")
	TSoftObjectPtr<UStaticMesh> RifleMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art|Phase8")
	FName RifleSocketName = TEXT("HandGrip_R");

	/** Sprint W — player rifle-carry animation takes (template Mannequins/Anims/Rifle + Death). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art|Phase8")
	FNightShiftPlayerAnimSet PlayerAnims;

	/** Sprint W — reference speeds (cm/s) at which the walk / jog clips play at rate 1.0. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art|Phase8")
	float PlayerWalkAnimRefSpeed = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art|Phase8")
	float PlayerJogAnimRefSpeed = 450.f;

	/** Below this ground speed the player plays Walk clips; above, Jog clips. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art|Phase8")
	float PlayerWalkJogSplitSpeed = 320.f;

	/** Sprint X — alien Run clip plays at rate 1.0 at this speed (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art|Phase8")
	float AlienRunAnimRefSpeed = 350.f;

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
	 * Sprint Q — server rack visual on rack-named cover volumes only (non-colliding).
	 * Default: /Game/Imported/Props/Office/SM_ServerRack (Dreadler CC-BY or other CC rack).
	 * Soft miss → greybox rack blocks only.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art|Phase8")
	TSoftObjectPtr<UStaticMesh> ServerRackPropMesh;

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
	 * Mid/late DESIGN numbers unchanged; grace is early-game only. Default 7s (Sprint V; was 4).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|EarlyGame")
	float SpawnGraceSeconds = 7.f; // Sprint V — was 4 (3–5 range); Richard: too hard off the bat

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
	float MinStartSeparationMeters = 24.f; // Sprint V — was 18

	/**
	 * Sprint V — after spawn grace ends, aliens may chase but cannot fire for this many seconds.
	 * Mid/late damage/accuracy unchanged.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|EarlyGame")
	float PostGraceAlienFireDelaySeconds = 1.5f;

	// -------------------------------------------------------------------------
	// Difficulty ramp (Sprint Y — Richard: "start super easy, then more spawning enemies and more
	// shooting at me"). Alpha = max(kills / RampKillsToMax, time / RampSecondsToMax), clamped 0..1.
	// Live alien target, alien accuracy and burst interval lerp Start → End/MaxLiveAliens on alpha.
	// bDifficultyRamp=false restores the flat DESIGN numbers (6 live, 30 %, 1.5 s).
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Ramp")
	bool bDifficultyRamp = true;

	/** Live aliens at match start; grows to MaxLiveAliens as the ramp alpha reaches 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Ramp", meta = (ClampMin = "1"))
	int32 RampStartLiveAliens = 2;

	/** Kills at which the ramp is fully on (KillsToWin 25 → the last 10 kills are full pressure). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Ramp", meta = (ClampMin = "1"))
	int32 RampKillsToMax = 15;

	/** Match seconds at which the ramp is fully on regardless of kills (slow players still get pushed). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Ramp", meta = (ClampMin = "1"))
	float RampSecondsToMax = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Ramp", meta = (ClampMin = "0", ClampMax = "1"))
	float RampStartAlienAccuracy = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Ramp", meta = (ClampMin = "0", ClampMax = "1"))
	float RampEndAlienAccuracy = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Ramp")
	float RampStartBurstIntervalSeconds = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Ramp")
	float RampEndBurstIntervalSeconds = 1.2f;

	// -------------------------------------------------------------------------
	// Wave progression (Sprint Z — Richard: "there should be a progression, not just a ramp to 100 %").
	// Numbered waves, each with a kill quota. Clearing a wave despawns the floor, restocks ammo/HP and
	// shows a breather banner; the next wave fields more, sharper aliens and unlocks variants.
	// Win = clear WavesToWin (0 = endless). When on, this supersedes the Sprint Y ramp and KillsToWin.
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Waves")
	bool bWaveProgression = true;

	/** Waves to clear for the win screen; 0 = endless (HUD shows the wave number only). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Waves", meta = (ClampMin = "0"))
	int32 WavesToWin = 8;

	/** Kill quota for wave N = WaveKillQuotaBase + WaveKillQuotaPerWave × N (3 + 1×N → 4, 5, 6 …). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Waves", meta = (ClampMin = "1"))
	int32 WaveKillQuotaBase = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Waves", meta = (ClampMin = "0"))
	int32 WaveKillQuotaPerWave = 1;

	/** Live aliens on wave N = WaveStartLiveAliens + WaveLiveAliensPerWave × (N − 1), capped at WaveMaxLiveAliens. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Waves", meta = (ClampMin = "1"))
	int32 WaveStartLiveAliens = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Waves", meta = (ClampMin = "0"))
	int32 WaveLiveAliensPerWave = 1;

	/** Pool size in wave mode (8 edge spawns → 8). MaxLiveAliens still applies when waves are off. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Waves", meta = (ClampMin = "1"))
	int32 WaveMaxLiveAliens = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Waves", meta = (ClampMin = "0", ClampMax = "1"))
	float WaveAccuracyStart = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Waves")
	float WaveAccuracyPerWave = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Waves", meta = (ClampMin = "0", ClampMax = "1"))
	float WaveAccuracyMax = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Waves")
	float WaveBurstIntervalStart = 3.0f;

	/** Seconds shaved off the burst interval per wave. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Waves")
	float WaveBurstIntervalPerWave = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Waves")
	float WaveBurstIntervalMin = 0.8f;

	/** cm/s added to AlienMoveSpeed per wave after the first. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Waves")
	float WaveMoveSpeedPerWave = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Waves")
	float WaveMoveSpeedMax = 520.f;

	/** Breather between waves: floor is empty, banner counts down, ammo/HP restock. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Waves")
	float WaveBreatherSeconds = 5.f;

	/** Grace (aliens idle at the edges) at the start of wave 2+; wave 1 uses SpawnGraceSeconds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Waves")
	float WaveStartGraceSeconds = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Waves")
	bool bWaveClearRefillsAmmo = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Waves")
	bool bWaveClearHeals = true;

	/** Big, slow, six body hits, red glow. Unlocks wave 3. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Waves")
	FAlienVariantTuning Brute;

	/** Small, fast, two body hits, quick bursts, cyan glow. Unlocks wave 5 (defaults set in the constructor). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Waves")
	FAlienVariantTuning Stalker;

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

	/**
	 * Sprint X — exposure compensation (EV) on the player camera. Project has auto-exposure off,
	 * so this is the single "lighten it up" knob on top of the greybox light intensities.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	float ExposureBiasEV = 0.6f;

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

	/** Sprint O — LoadSynchronous once; reuse cached meshes (avoids per-alien PIE hitch). */
	void ResolvePhase8LoadedMeshes();

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Art|Phase8|Cache")
	TObjectPtr<UStaticMesh> CachedAlienBodyMesh;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Art|Phase8|Cache")
	TObjectPtr<UStaticMesh> CachedAlienHeadMesh;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Art|Phase8|Cache")
	TObjectPtr<USkeletalMesh> CachedAlienSkeletalMesh;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Art|Phase8|Cache")
	TObjectPtr<UStaticMesh> CachedCoverPropMesh;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Art|Phase8|Cache")
	TObjectPtr<UStaticMesh> CachedDeskPropMesh;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Art|Phase8|Cache")
	TObjectPtr<UStaticMesh> CachedChairPropMesh;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Art|Phase8|Cache")
	TObjectPtr<UStaticMesh> CachedServerRackPropMesh;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Art|Phase8|Cache")
	TObjectPtr<UStaticMesh> CachedFluorescentLightMesh;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Art|Phase8|Cache")
	TObjectPtr<USkeletalMesh> CachedPlayerSkeletalMesh;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Art|Phase8|Cache")
	TObjectPtr<UStaticMesh> CachedRifleMesh;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Art|Phase8|Cache")
	FNightShiftPlayerAnimCache CachedPlayerAnims;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Art|Phase8|Cache")
	FNightShiftAlienAnimCache CachedAlienAnims;

	bool bPhase8MeshesResolved = false;
};

