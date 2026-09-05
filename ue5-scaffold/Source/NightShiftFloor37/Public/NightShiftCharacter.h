// Night Shift — Floor 37 | Move, OTS camera (right shoulder + Q swap), health, recoil
// Phase 1: API complete enough to wire Enhanced Input.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "NightShiftCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class URifleComponent;
class UGameConfig;
class UInputMappingContext;
class UInputAction;
class UArenaCollision;
class UCameraShakeBase;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamaged, float, Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, Health, float, MaxHealth);

/**
 * Player pawn: CMC move/sprint/jump, over-the-shoulder camera (right default, Q swap),
 * health + regen (10/s after 5s), fall damage (>6m), recoil consume from rifle.
 * PERFORMANCE: no per-frame allocs in Tick / Move / Look.
 */
UCLASS()
class NIGHTSHIFTFLOOR37_API ANightShiftCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ANightShiftCharacter();

	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;

	// ----- Components -----

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<URifleComponent> Rifle;

	/** Greybox body (engine cylinder) so the player reads in 3rd person without art. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UArenaCollision> ArenaCollision;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	TObjectPtr<UGameConfig> GameConfig;

	// ----- Enhanced Input assets (assign in BP / defaults) -----

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ShoulderSwapAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> PauseAction;

	// ----- Health / state -----

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	float Health = 100.f;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	float TimeSinceLastDamage = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	bool bRightShoulder = true;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bWantsSprint = false;

	/** Last grounded Z (cm) for fall-damage check. Snapshotted on leave-ground. */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float LastGroundedZ = 0.f;

	/**
	 * Residual recoil (degrees). Kick adds here and immediately snaps controller;
	 * Tick recovers toward zero (pulls camera back) via RecoilRecoverySpeed.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Recoil")
	FVector2D RecoilOffsetDegrees = FVector2D::ZeroVector;

	// ----- Feedback (HUD / vignette / shake) -----
	// Vignette: bind WBP red-edge overlay to OnDamaged (Editor-only UMG).
	// Camera shake: assign DamageCameraShake — played in TakeDamage when set.

	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnDamaged OnDamaged;

	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnDied OnDied;

	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnHealthChanged OnHealthChanged;

	/** Optional camera shake played on TakeDamage (EditAnywhere — assign UCameraShakeBase subclass in BP). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	TSubclassOf<UCameraShakeBase> DamageCameraShake;

	// ----- Input handlers (wire to Enhanced Input) -----

	UFUNCTION(BlueprintCallable, Category = "Input")
	void Move(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void Look(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void StartSprint();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void StopSprint();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SwapShoulder();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void StopFire();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void RequestReload();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void RequestPause();

	/** Space: jump, or short mantle if a ledge is within MantleReach (DESIGN Jump/mantle). */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void TryJumpOrMantle();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MantleReachCm = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MantleHeightCm = 120.f;

	// ----- Combat / health -----

	UFUNCTION(BlueprintCallable, Category = "Health")
	void ApplyHeal(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void SoftResetPlayerState();

	/** Apply GameConfig after ResolveOrCreate (movement, capsule, rifle). */
	UFUNCTION(BlueprintCallable, Category = "Config")
	void ApplyResolvedGameConfig();

	UFUNCTION(BlueprintCallable, Category = "Recoil")
	void AddRecoilKick(float PitchDegrees, float YawDegrees);

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsAlive() const { return Health > 0.f; }

	/** True while AArenaGameMode reports the match paused (Esc). */
	bool IsMatchPaused() const;

	UFUNCTION(BlueprintPure, Category = "Camera")
	FVector GetAimOrigin() const;

	UFUNCTION(BlueprintPure, Category = "Camera")
	FVector GetAimDirection() const;

protected:
	/** Create Input Actions + a mapping context in code when no Editor assets are assigned. */
	void BuildRuntimeInputDefaults();
	/** Add DefaultMappingContext to the local player's Enhanced Input subsystem (idempotent). */
	void EnsureMappingContext();
	void ApplyConfigToMovement();
	void UpdateRegen(float DeltaSeconds);
	void UpdateRecoilRecovery(float DeltaSeconds);
	void UpdateSprintSpeed();
	void ApplyShoulderOffset();
	bool TryMantleOverLedge();
	void BroadcastHealthChanged();
	void SnapshotGroundedZIfNeeded();

	/** Eye / capsule ~1.8 m (DESIGN). */
	void ConfigureCapsuleFromConfig();

	/** Tracks IsMovingOnGround edge for fall-height snapshot when MovementModeChanged is not enough. */
	bool bWasMovingOnGround = true;
};
