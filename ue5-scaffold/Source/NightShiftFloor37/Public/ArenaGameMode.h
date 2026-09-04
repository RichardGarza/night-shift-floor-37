// Night Shift — Floor 37 | Match, timer, kills, win/lose, soft restart
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ArenaGameMode.generated.h"

class UGameConfig;
class AOfficeArena;
class AAlienBot;
class UHUDWidget;

UENUM(BlueprintType)
enum class EArenaMatchState : uint8
{
	WaitingToStart UMETA(DisplayName = "WaitingToStart"),
	InProgress     UMETA(DisplayName = "InProgress"),
	Won            UMETA(DisplayName = "Won"),
	Lost           UMETA(DisplayName = "Lost")
};

/**
 * Owns match flow: kill count, timer, win at KillsToWin, death → restart prompt, soft reset.
 * Soft restart resets HP/ammo/kills/timer/alien state/player transform without unloading the level.
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

	UPROPERTY(BlueprintReadOnly, Category = "Match")
	EArenaMatchState MatchState = EArenaMatchState::WaitingToStart;

	UPROPERTY(BlueprintReadOnly, Category = "Match")
	int32 KillCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Match")
	float MatchTimeSeconds = 0.f;

	UFUNCTION(BlueprintCallable, Category = "Match")
	void StartMatch();

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
	bool HasWon() const { return MatchState == EArenaMatchState::Won; }

protected:
	void CheckWinCondition();
	void EnsureAlienPopulation();
	void ClampDelta(float& DeltaSeconds) const;

	UPROPERTY()
	TObjectPtr<AOfficeArena> CachedArena;

	UPROPERTY()
	TObjectPtr<UHUDWidget> HUDWidget;

	bool bMatchPaused = false;
};
