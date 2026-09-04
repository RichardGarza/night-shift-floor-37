// Night Shift — Floor 37 | Move, strafe, burst, flash, death/respawn
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AlienBot.generated.h"

class UGameConfig;
class ANightShiftCharacter;
class UArenaCollision;
class AOfficeArena;

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

	/** Remaining hit-flash time in seconds (DESIGN: 80 ms → 0.08 s). */
	UPROPERTY(BlueprintReadOnly, Category = "FX")
	float HitFlashTimeRemaining = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FX")
	bool bIsFlashing = false;

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

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsLocationOnHead(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsHeadBone(FName BoneName) const;

	/** True while death→respawn timer is running (GameMode must not double-activate). */
	UFUNCTION(BlueprintPure, Category = "AI")
	bool IsRespawnPending() const;

protected:
	void UpdateAI(float DeltaSeconds);
	void ChasePlayer(float DeltaSeconds);
	void StrafeAndBurst(float DeltaSeconds);
	void TryBurstShot();
	void Die();
	void ScheduleRespawn();
	void PlayHitFlash();
	void UpdateHitFlash(float DeltaSeconds);
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

	/** Alternating lateral sign when forward steer probe is blocked. */
	float SteerSideSign = 1.f;

	FTimerHandle RespawnTimerHandle;

	/** Set while RespawnTimerHandle is armed; cleared on Activate/SoftDespawn/SoftReset. */
	bool bRespawnScheduled = false;

	/** Reused chase / LOS traces — no per-frame heap in AI tick. */
	mutable FHitResult SteerHitScratch;
	mutable FHitResult LosHitScratch;
};
