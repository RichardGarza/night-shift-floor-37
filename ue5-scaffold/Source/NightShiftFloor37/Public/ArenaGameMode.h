// Night Shift — Floor 37 | Match, timer, kills, win/lose, soft restart
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ArenaGameMode.generated.h"

class UGameConfig;
class AOfficeArena;
class AAlienBot;
class UHUDWidget;
class ANightShiftCharacter;

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

protected:
	void SetMatchState(EArenaMatchState NewState);
	void CheckWinCondition();
	void EnsureAlienPopulation();
	void ClampDelta(float& DeltaSeconds) const;
	void FindOrCacheArena();
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
};
