// Night Shift — Floor 37 | Move, strafe, burst, flash, death/respawn
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameConfig.h"
#include "AlienBot.generated.h"

class UStaticMeshComponent;
class USkeletalMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class AFXPoolManager;
class UAnimSequence;
class UMaterialInterface;
class USkeletalMesh;

class UGameConfig;
class ANightShiftCharacter;
class UArenaCollision;
class AOfficeArena;
class AArenaGameMode;

UENUM(BlueprintType)
enum class EAlienCombatState : uint8
{
	Idle,
	Chase,
	StrafeBurst,
	Dead
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAlienHitFlash, bool, bFlashing);

/**
 * Capsule alien: 4 m/s chase, ≤12 m combat (strafe + 3-round burst / 1.5s, 30% accuracy, 10 dmg).
 * Kill: 3 body or 2 head. Respawn 3 s. Head = top 25% of capsule.
 * Burst fires with short intra-shot delay (not all in one Tick). Hit flash ~80 ms wall time.
 */
UCLASS()
class NIGHTSHIFTFLOOR37_API AAlienBot : public ACharacter
{
	GENERATED_BODY()

public:
	AAlienBot();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	TObjectPtr<UGameConfig> GameConfig;

	/** Greybox body + head (engine cylinder / sphere) in one bright bio colour. Head = top 25%. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FLinearColor BodyColor = FLinearColor(0.35f, 1.0f, 0.3f);

	/** Point light pulsed by the hit flash so the 80 ms white pop reads from any angle. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UPointLightComponent> FlashLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UArenaCollision> ArenaCollision;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	EAlienCombatState CombatState = EAlienCombatState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	int32 BodyHitCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	int32 HeadHitCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsAlive = true;

	/** Sprint X — true once AlienSkeletalMesh replaced the greybox / static body. */
	UPROPERTY(BlueprintReadOnly, Category = "Visual")
	bool bSkeletalActive = false;

	/** Sprint Z — Grunt / Brute / Stalker; set by the GameMode before ActivateAtSpawn. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	EAlienVariant Variant = EAlienVariant::Grunt;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetVariant(EAlienVariant NewVariant);

	/** Tuning for Brute / Stalker; nullptr for the Grunt (DESIGN numbers apply). */
	const FAlienVariantTuning* GetVariantTuning() const;

	/** Remaining hit-flash time in seconds (DESIGN: 80 ms → 0.08 s). */
	UPROPERTY(BlueprintReadOnly, Category = "FX")
	float HitFlashTimeRemaining = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FX")
	bool bIsFlashing = false;

	/**
	 * Material/BP-readable flash intensity 0→1 while flashing (DESIGN: white flash 80 ms).
	 * Bind mesh emissive to this / OnHitFlash in Editor. No mesh materials required in C++.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "FX")
	float HitFlashAlpha = 0.f;

	/** Broadcast when flash starts (true) or expires (false). */
	UPROPERTY(BlueprintAssignable, Category = "FX")
	FOnAlienHitFlash OnHitFlash;

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetTarget(ANightShiftCharacter* InTarget);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void ActivateAtSpawn(const FTransform& SpawnTransform);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SoftDespawn();

	/** Match soft-restart: clear timers, reset combat, despawn (GameMode redistributes). */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SoftReset();

	/** Called by respawn timer or GameMode — farthest edge spawn from player. */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void PerformRespawn();

	/**
	 * Phase 8 — apply UGameConfig soft mesh overrides when present; else keep greybox
	 * cylinder/sphere. Safe to call after GameConfig is assigned (BeginPlay / pool activate).
	 */
	UFUNCTION(BlueprintCallable, Category = "Visual|Phase8")
	void ApplyConfiguredMeshes();

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsLocationOnHead(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsHeadBone(FName BoneName) const;

	/** True while death→respawn timer is running (GameMode must not double-activate). */
	UFUNCTION(BlueprintPure, Category = "AI")
	bool IsRespawnPending() const;

	/** When true, ChasePlayer tries AI MoveTo before steering fallback (needs NavMesh + AIController). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	bool bPreferNavMeshMoveTo = false;

protected:
	void UpdateAI(float DeltaSeconds);
	void ChasePlayer(float DeltaSeconds);
	bool TryNavMeshMoveToTarget();
	void StrafeAndBurst(float DeltaSeconds);
	void TryBurstShot();
	void Die();
	void ScheduleRespawn();
	void PlayHitFlash();
	void UpdateHitFlash(float DeltaSeconds);
	/** Push HitFlashAlpha into the greybox materials (white flash). */
	void ApplyFlashToMaterials();
	void InvalidateFlashMIDs();
	void ApplyBioFlashColorToMID(UMaterialInstanceDynamic* MID, const FLinearColor& Color) const;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BodyMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> HeadMID;

	/** Per-slot MIDs on GetMesh() when AlienSkeletalMesh is active (Sprint I). */
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> SkelMIDs;

	/** Shared tracer / muzzle-light pool (found once). */
	TWeakObjectPtr<AFXPoolManager> FXPool;
	bool HasLineOfSightToTarget() const;
	float DistanceToTargetMeters() const;
	AOfficeArena* FindArena() const;
	FVector GetPlayerLocationOrSelf() const;

	UPROPERTY()
	TWeakObjectPtr<ANightShiftCharacter> TargetPlayer;

	/** Seconds until next burst may start (DESIGN: AlienBurstIntervalSeconds = 1.5). */
	float BurstCooldownRemaining = 0.f;

	/** Shots left in the current burst (0 = idle between bursts). */
	int32 BurstShotsRemaining = 0;

	/** Seconds until the next intra-burst shot fires (~0.08–0.1). */
	float BurstIntraShotRemaining = 0.f;

	float StrafeSign = 1.f;

	/** Lateral steer sign around an obstacle; held for AlienSteerCommitSeconds (Sprint Y — no per-frame flip). */
	float SteerSideSign = 1.f;
	float SteerCommitRemaining = 0.f;

	/** Sprint Y — cached GameMode (ramp-scaled accuracy / burst interval, fire lock). */
	mutable TWeakObjectPtr<AArenaGameMode> CachedGameMode;
	AArenaGameMode* GetArenaGameMode() const;
	/** Yaw toward the player at AlienFaceTargetTurnRateDegPerSec; returns remaining yaw error in degrees. */
	float FaceTarget(float DeltaSeconds);

	FTimerHandle RespawnTimerHandle;

	/** Set while RespawnTimerHandle is armed; cleared on Activate/SoftDespawn/SoftReset. */
	bool bRespawnScheduled = false;

	/** Reused chase / LOS traces — no per-frame heap in AI tick. */
	mutable FHitResult SteerHitScratch;
	mutable FHitResult LosHitScratch;

	// ----- Sprint X code-driven animation + death / flash on the skeletal body -----
	UPROPERTY() TObjectPtr<UAnimSequence> CurrentAnim;
	/** Original per-slot materials of GetMesh(), restored after the white flash swap. */
	UPROPERTY() TArray<TObjectPtr<UMaterialInterface>> SkelOriginalMaterials;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FlashSwapMID;
	bool bFlashSwapActive = false;
	float AttackAnimRemaining = 0.f;
	/** > 0 while the death clip plays; actor hides when it reaches 0 (respawn timer runs in parallel). */
	float DeathHideRemaining = 0.f;
	void PlayAlienAnim(UAnimSequence* Seq, bool bLoop, float Rate = 1.f);
	void UpdateAlienAnim(float DeltaSeconds);
	/** Scale + place the skeletal mesh and refit the capsule to its bounds (feet at capsule bottom). */
	void FitSkeletalBody(USkeletalMesh* Skel);
	/** Sprint Z — refit for the variant scale, set speed and the always-on glow. Call after ApplyConfiguredMeshes. */
	void ApplyVariantPresentation();
	float AppliedScaleMul = 1.f;
	/** Capsule half-height a Grunt would have (spawn transforms assume it). */
	float GruntHalfHeight = 88.f;
	float GlowBaseIntensity = 0.f;
	void BeginFlashSwap();
	void EndFlashSwap();
};
