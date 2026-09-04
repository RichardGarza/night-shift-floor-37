#include "ArenaGameMode.h"
#include "GameConfig.h"
#include "OfficeArena.h"
#include "AlienBot.h"
#include "NightShiftFloor37.h"
#include "EngineUtils.h"

AArenaGameMode::AArenaGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	// DefaultPawnClass / HUDClass — set in Blueprint or project defaults after drop-in.
}

void AArenaGameMode::BeginPlay()
{
	Super::BeginPlay();
	// TODO: Resolve GameConfig from Content/Data if unset.
	// TODO: Find AOfficeArena in world; cache spawn points.
	// TODO: Create UHUDWidget and show "Click to play".
	UE_LOG(LogNightShift, Log, TEXT("AArenaGameMode::BeginPlay — waiting to start (win @ %d kills)"),
		GameConfig ? GameConfig->KillsToWin : 25);
	MatchState = EArenaMatchState::WaitingToStart;
}

void AArenaGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ClampDelta(DeltaSeconds);
	if (MatchState != EArenaMatchState::InProgress || bMatchPaused)
	{
		return;
	}
	MatchTimeSeconds += DeltaSeconds;
	EnsureAlienPopulation();
}

void AArenaGameMode::ClampDelta(float& DeltaSeconds) const
{
	// DESIGN: treat spikes above ~50 ms as 50 ms
	const float MaxDt = GameConfig ? GameConfig->MaxDeltaTimeClampSeconds : 0.05f;
	if (DeltaSeconds > MaxDt)
	{
		DeltaSeconds = MaxDt;
	}
}

void AArenaGameMode::StartMatch()
{
	KillCount = 0;
	MatchTimeSeconds = 0.f;
	MatchState = EArenaMatchState::InProgress;
	EnsureAlienPopulation();
	UE_LOG(LogNightShift, Log, TEXT("Match started."));
}

void AArenaGameMode::RegisterKill(AActor* /*Victim*/)
{
	if (MatchState != EArenaMatchState::InProgress)
	{
		return;
	}
	++KillCount;
	CheckWinCondition();
}

void AArenaGameMode::NotifyPlayerDied()
{
	MatchState = EArenaMatchState::Lost;
	// TODO: HUD — "You died — click to restart"
	UE_LOG(LogNightShift, Log, TEXT("Player died."));
}

void AArenaGameMode::SoftRestart()
{
	// DESIGN: soft reset without unloading level
	KillCount = 0;
	MatchTimeSeconds = 0.f;
	MatchState = EArenaMatchState::WaitingToStart;
	// TODO: Reset player HP/ammo/transform via character API
	// TODO: Despawn/reset all aliens; clear pools
	UE_LOG(LogNightShift, Log, TEXT("SoftRestart complete."));
}

void AArenaGameMode::PauseMatch(bool bPause)
{
	bMatchPaused = bPause;
	// Esc → pause / unlock (DESIGN input)
}

void AArenaGameMode::CheckWinCondition()
{
	const int32 Need = GameConfig ? GameConfig->KillsToWin : 25;
	if (KillCount >= Need)
	{
		MatchState = EArenaMatchState::Won;
		// TODO: HUD win screen with MatchTimeSeconds
		UE_LOG(LogNightShift, Log, TEXT("WIN — %d kills in %.2fs"), KillCount, MatchTimeSeconds);
	}
}

void AArenaGameMode::EnsureAlienPopulation()
{
	// TODO: Count live AAlienBot; spawn from farthest of 8 points until MaxLiveAliens (6)
}
