// Night Shift — Floor 37 | Move, strafe, burst, flash, death/respawn
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AlienBot.generated.h"

class UGameConfig;
class ANightShiftCharacter;
class UArenaCollision;

UENUM(BlueprintType)
enum class EAlienCombatState : uint8
{
	Idle,
	Chase,
	StrafeBurst,
	Dead
};

/**
 * Capsule alien: 4 m/s chase, ≤12 m combat (strafe + 3-round burst / 1.5s, 30% accuracy, 10 dmg).
 * Kill: 3 body or 2 head. Respawn 3 s. Head = top 25% of capsule.
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

	UPROPERTY(BlueprintReadOnly, Category = "FX")
	float HitFlashTimeRemaining = 0.f;

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetTarget(ANightShiftCharacter* InTarget);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void ActivateAtSpawn(const FTransform& SpawnTransform);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SoftDespawn();

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsLocationOnHead(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsHeadBone(FName BoneName) const;

protected:
	void UpdateAI(float DeltaSeconds);
	void ChasePlayer(float DeltaSeconds);
	void StrafeAndBurst(float DeltaSeconds);
	void TryBurstShot();
	void Die();
	void ScheduleRespawn();
	void PlayHitFlash();
	bool HasLineOfSightToTarget() const;
	float DistanceToTargetMeters() const;

	UPROPERTY()
	TWeakObjectPtr<ANightShiftCharacter> TargetPlayer;

	float BurstCooldownRemaining = 0.f;
	int32 BurstShotsRemaining = 0;
	float StrafeSign = 1.f;
	FTimerHandle RespawnTimerHandle;
};
