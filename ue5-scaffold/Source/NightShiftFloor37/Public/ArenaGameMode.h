// Night Shift — Floor 37 | Match, timer, kills, win/lose, soft restart
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameConfig.h"
#include "ArenaGameMode.generated.h"

class UGameConfig;
class AOfficeArena;
class AAlienBot;
class UHUDWidget;
class ANightShiftCharacter;
class UAudioComponent;
class ANightShiftDemoPilot;

UENUM(BlueprintType)
enum class EArenaMatchState : uint8
{
	WaitingToStart UMETA(DisplayName = "WaitingToStart"),
	InProgress     UMETA(DisplayName = "InProgress"),
	Won            UMETA(DisplayName = "Won"),
	Lost           UMETA(DisplayName = "Lost")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchStateChanged, EArenaMatchState, NewState);

/**
 * Owns match flow: kill count, timer, win at KillsToWin, death → restart prompt, soft reset.
 * Soft restart resets HP/ammo/kills/timer/alien state/player transform without unloading the level.
 * Maintains a pool of up to MaxLiveAliens (6) AAlienBot actors.
 */
UCLASS()
class NIGHTSHIFTFLOOR37_API AArenaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AArenaGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	/** Falls back to a spawned APlayerStart beside the arena when the map has none. */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	TObjectPtr<UGameConfig> GameConfig;

	/** Optional Editor-wired arena; auto-found on BeginPlay if unset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena")
	TObjectPtr<AOfficeArena> CachedArena;

	/** Class used when spawning pool bots (defaults to AAlienBot). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aliens")
	TSubclassOf<AAlienBot> AlienBotClass;

	/**
	 * UMG HUD widget class (assign WBP_NightShiftHUD in Editor).
	 * If unset, match logic still runs and a one-time warning is logged.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	TSubclassOf<UHUDWidget> HUDWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "Match")
	EArenaMatchState MatchState = EArenaMatchState::WaitingToStart;

	UPROPERTY(BlueprintReadOnly, Category = "Match")
	int32 KillCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Match")
	float MatchTimeSeconds = 0.f;

	/** Sprint Z — 1-based wave number (wave mode). Public so tests / debug can jump waves. */
	UPROPERTY(BlueprintReadOnly, Category = "Match|Waves")
	int32 CurrentWave = 1;

	/** Kills scored on the current wave. */
	UPROPERTY(BlueprintReadOnly, Category = "Match|Waves")
	int32 WaveKills = 0;

	UPROPERTY(BlueprintAssignable, Category = "Match|Events")
	FOnMatchStateChanged OnMatchStateChanged;

	UFUNCTION(BlueprintCallable, Category = "Match")
	void StartMatch();

	/**
	 * Click-to-play / click-to-restart entry.
	 * WaitingToStart → StartMatch.
	 * Lost / Won → SoftRestart + StartMatch (one click).
	 */
	UFUNCTION(BlueprintCallable, Category = "Match")
	void RequestStartOrRestart();

	UFUNCTION(BlueprintCallable, Category = "Match")
	void RegisterKill(AActor* Victim);

	UFUNCTION(BlueprintCallable, Category = "Match")
	void NotifyPlayerDied();

	/** Soft reset: HP, ammo, kills, timer, aliens, player transform. Same level. */
	UFUNCTION(BlueprintCallable, Category = "Match")
	void SoftRestart();

	UFUNCTION(BlueprintCallable, Category = "Match")
	void PauseMatch(bool bPause);

	UFUNCTION(BlueprintPure, Category = "Match")
	bool IsMatchPaused() const { return bMatchPaused; }

	UFUNCTION(BlueprintPure, Category = "Match")
	bool HasWon() const { return MatchState == EArenaMatchState::Won; }

	UFUNCTION(BlueprintPure, Category = "Arena")
	AOfficeArena* GetOfficeArena() const { return CachedArena; }

	UFUNCTION(BlueprintPure, Category = "HUD")
	UHUDWidget* GetHUDWidget() const { return HUDWidget; }

	/**
	 * Respawn a dead pool bot at farthest edge spawn from the player.
	 * Returns true if activated; false if arena/player missing (caller may fall back).
	 */
	UFUNCTION(BlueprintCallable, Category = "Aliens")
	bool RespawnAlien(AAlienBot* Bot);

	UFUNCTION(BlueprintPure, Category = "Aliens")
	int32 GetLiveAlienCount() const;

	/** True while early-game spawn grace is counting down (Sprint C). */
	UFUNCTION(BlueprintPure, Category = "Match|EarlyGame")
	bool IsSpawnGraceActive() const { return SpawnGraceRemaining > 0.f && MatchState == EArenaMatchState::InProgress; }

	/** Grace, post-grace fire lock, or the between-wave breather — aliens must not shoot. */
	UFUNCTION(BlueprintPure, Category = "Match|EarlyGame")
	bool IsAlienFireLocked() const
	{
		return MatchState == EArenaMatchState::InProgress
			&& (SpawnGraceRemaining > 0.f || AlienFireLockRemaining > 0.f || bInWaveBreak);
	}

	UFUNCTION(BlueprintPure, Category = "Match|EarlyGame")
	float GetSpawnGraceRemaining() const { return SpawnGraceRemaining; }

	// ----- Sprint Y difficulty ramp -----

	/** 0 at match start → 1 when kills or time hit the ramp caps. Always 1 when the ramp is off. */
	UFUNCTION(BlueprintPure, Category = "Match|Ramp")
	float GetDifficultyAlpha() const;

	/** Live aliens the population keeper aims for right now (RampStartLiveAliens → MaxLiveAliens). */
	UFUNCTION(BlueprintPure, Category = "Match|Ramp")
	int32 GetTargetLiveAliens() const;

	/** Alien hit chance at the current ramp alpha. */
	UFUNCTION(BlueprintPure, Category = "Match|Ramp")
	float GetAlienAccuracy() const;

	/** Seconds between alien bursts at the current ramp alpha. */
	UFUNCTION(BlueprintPure, Category = "Match|Ramp")
	float GetAlienBurstInterval() const;

	/** 1..5 HUD bucket of the ramp alpha. */
	UFUNCTION(BlueprintPure, Category = "Match|Ramp")
	int32 GetThreatTier() const;

	// ----- Sprint Z wave progression -----

	UFUNCTION(BlueprintPure, Category = "Match|Waves")
	bool IsWaveProgression() const;

	/** Kills needed to clear CurrentWave. */
	UFUNCTION(BlueprintPure, Category = "Match|Waves")
	int32 GetWaveKillQuota() const;

	/** WavesToWin from config (0 = endless). */
	UFUNCTION(BlueprintPure, Category = "Match|Waves")
	int32 GetWavesToWin() const;

	UFUNCTION(BlueprintPure, Category = "Match|Waves")
	bool IsInWaveBreak() const { return bInWaveBreak; }

	UFUNCTION(BlueprintPure, Category = "Match|Waves")
	float GetWaveBreakRemaining() const { return WaveBreakRemaining; }

	/** Alien chase speed (cm/s) for the current wave. */
	UFUNCTION(BlueprintPure, Category = "Match|Waves")
	float GetAlienMoveSpeed() const;

	/** Variant for the next activation: fills Brute / Stalker quotas once their wave unlocks, else Grunt. */
	UFUNCTION(BlueprintPure, Category = "Match|Waves")
	EAlienVariant PickVariantForSpawn() const;

	/** Force-clear the current wave (debug / self-test). */
	UFUNCTION(BlueprintCallable, Category = "Match|Waves")
	void DebugClearWave();

	// ----- Sprint AG hands-off demo -----

	UFUNCTION(BlueprintPure, Category = "Match|Demo")
	bool IsDemoActive() const { return DemoPilot != nullptr; }

	/** Start the autopilot demo (soft-restarts into a live match driven by ANightShiftDemoPilot). */
	UFUNCTION(BlueprintCallable, Category = "Match|Demo")
	void StartDemo();

	/** End the demo: back to "Click to play" (bHandToPlayer = a human clicked → straight into a fresh match). */
	UFUNCTION(BlueprintCallable, Category = "Match|Demo")
	void EndDemo(bool bHandToPlayer);

	/** Sprint AF — looping office hum started in BeginPlay (null when audio is off or the asset is missing). */
	UFUNCTION(BlueprintPure, Category = "Audio")
	UAudioComponent* GetAmbientLoop() const { return AmbientLoop; }

protected:
	/** Begin grace timer + place player on farthest edge spawn from aliens / push aliens out. */
	void BeginSpawnGraceAndSafeStart();
	void ApplySaferStartSpacing();
	/** Sprint M — yaw control toward nearest live alien (else atrium) after safer-start; no position change. */
	void OrientPlayerTowardStartFocus();
	void SetMatchState(EArenaMatchState NewState);
	void CheckWinCondition();
	void EnsureAlienPopulation();
	/** Sprint Z — wave quota met: despawn the floor, restock, start the breather (or win on the last wave). */
	void OnWaveCleared();
	/** Sprint Z — breather over: advance the wave, short grace, repopulate. */
	void StartNextWave();
	/** Sprint Z — refresh the between-wave banner when the countdown second changes. */
	void UpdateWaveBreakBanner();
	int32 GetPoolSize() const;
	/** Clamp player + live bots into AOfficeArena bounds/ceiling each tick. */
	void EnforceArenaBounds();
	void ClampDelta(float& DeltaSeconds) const;
	/** Auto-resolve GameConfig (asset or DESIGN defaults) and push to bots/player/rifle/collision. */
	void ResolveAndPropagateGameConfig();
	void FindOrCacheArena();
	/** Spawn AOfficeArena / AFXPoolManager if the level has none, so an empty map still plays. */
	void EnsureWorldActors();
	void BuildAlienPool();
	void SoftRestartAlienPool();
	/** @param bShowPromptIfWaiting Show "Click to play" when leaving match in WaitingToStart. */
	void SoftRestartInternal(bool bShowPromptIfWaiting);
	void CreateAndBindHUD();
	void EnsureHUDBound();
	void UpdatePlayerInputMode();
	void RecordStartTransform();
	void ResetPlayerTransform();
	FVector GetPlayerLocation() const;
	ANightShiftCharacter* GetPlayerCharacter() const;

	UPROPERTY()
	TArray<TObjectPtr<AAlienBot>> AlienPool;

	UPROPERTY()
	TObjectPtr<UHUDWidget> HUDWidget;

	/** Player transform recorded on BeginPlay (fallback if no APlayerStart). */
	FTransform StartTransform = FTransform::Identity;
	bool bHasStartTransform = false;

	bool bMatchPaused = false;
	bool bLoggedMissingHUDClass = false;

	/** Countdown after StartMatch — aliens idle/no-fire while > 0 (Sprint C). */
	float SpawnGraceRemaining = 0.f;

	/** Countdown after grace — chase OK, fire blocked (Sprint V). */
	float AlienFireLockRemaining = 0.f;

	/** Last logged threat tier so the ramp only logs on change. */
	int32 LoggedThreatTier = 0;

	/** Sprint Z — between-wave breather. */
	bool bInWaveBreak = false;
	float WaveBreakRemaining = 0.f;
	int32 LastBannerSecond = -1;
	/** -NightShiftStartWave=N (screenshots / smoke runs at a later wave). 0 = unset. */
	int32 DebugStartWave = 0;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> AmbientLoop;

	UPROPERTY(Transient)
	TObjectPtr<ANightShiftDemoPilot> DemoPilot;
	float DemoSecondsOverride = 0.f;
	float IdleAtPrompt = 0.f;
	FTimerHandle DemoRespawnTimer;
	void DemoRespawnAfterDeath();

	/** Sprint AE — "Wave N" banner at wave start; cleared by timer while still in play. */
	FTimerHandle WaveBannerTimer;
	void ShowWaveStartBanner();
	void ClearWaveStartBanner();
};
