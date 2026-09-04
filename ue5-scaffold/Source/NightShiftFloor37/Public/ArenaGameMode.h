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

	UFUNCTION(BlueprintPure, Category = "Arena")
	AOfficeArena* GetOfficeArena() const { return CachedArena; }

	/**
	 * Respawn a dead pool bot at farthest edge spawn from the player.
	 * Returns true if activated; false if arena/player missing (caller may fall back).
	 */
	UFUNCTION(BlueprintCallable, Category = "Aliens")
	bool RespawnAlien(AAlienBot* Bot);

	UFUNCTION(BlueprintPure, Category = "Aliens")
	int32 GetLiveAlienCount() const;

protected:
	void CheckWinCondition();
	void EnsureAlienPopulation();
	void ClampDelta(float& DeltaSeconds) const;
	void FindOrCacheArena();
	void BuildAlienPool();
	void SoftRestartAlienPool();
	FVector GetPlayerLocation() const;
	ANightShiftCharacter* GetPlayerCharacter() const;

	UPROPERTY()
	TArray<TObjectPtr<AAlienBot>> AlienPool;

	UPROPERTY()
	TObjectPtr<UHUDWidget> HUDWidget;

	bool bMatchPaused = false;
};
