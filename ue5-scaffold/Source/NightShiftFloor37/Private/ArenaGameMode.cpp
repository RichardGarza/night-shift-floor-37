#include "ArenaGameMode.h"
#include "GameConfig.h"
#include "OfficeArena.h"
#include "AlienBot.h"
#include "NightShiftCharacter.h"
#include "RifleComponent.h"
#include "HUDWidget.h"
#include "NightShiftFloor37.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"

AArenaGameMode::AArenaGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	AlienBotClass = AAlienBot::StaticClass();
	// DefaultPawnClass / HUDClass — set in Blueprint or project defaults after drop-in.
}

void AArenaGameMode::BeginPlay()
{
	Super::BeginPlay();
	// TODO: Resolve GameConfig from Content/Data if unset.
	FindOrCacheArena();
	BuildAlienPool();
	RecordStartTransform();
	CreateAndBindHUD();

	SetMatchState(EArenaMatchState::WaitingToStart);
	UpdatePlayerInputMode();

	UE_LOG(LogNightShift, Log, TEXT("AArenaGameMode::BeginPlay — waiting to start (win @ %d kills, pool %d)"),
		GameConfig ? GameConfig->KillsToWin : 25,
		AlienPool.Num());
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

void AArenaGameMode::SetMatchState(EArenaMatchState NewState)
{
	if (MatchState == NewState)
	{
		return;
	}
	MatchState = NewState;
	OnMatchStateChanged.Broadcast(NewState);
}

void AArenaGameMode::FindOrCacheArena()
{
	if (CachedArena)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (TActorIterator<AOfficeArena> It(World); It; ++It)
	{
		CachedArena = *It;
		UE_LOG(LogNightShift, Log, TEXT("AArenaGameMode: cached AOfficeArena %s"), *CachedArena->GetName());
		return;
	}
	UE_LOG(LogNightShift, Warning, TEXT("AArenaGameMode: no AOfficeArena in world — place one or set CachedArena."));
}

void AArenaGameMode::BuildAlienPool()
{
	const int32 MaxLive = GameConfig ? GameConfig->MaxLiveAliens : 6;
	AlienPool.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Adopt any level-placed bots first (Editor prototypes).
	for (TActorIterator<AAlienBot> It(World); It; ++It)
	{
		AAlienBot* Bot = *It;
		if (!Bot)
		{
			continue;
		}
		if (GameConfig && !Bot->GameConfig)
		{
			Bot->GameConfig = GameConfig;
		}
		Bot->SoftDespawn();
		AlienPool.Add(Bot);
		if (AlienPool.Num() >= MaxLive)
		{
			break;
		}
	}

	// Spawn the rest into the pool (hidden until StartMatch / EnsureAlienPopulation).
	TSubclassOf<AAlienBot> ClassToSpawn = AlienBotClass ? AlienBotClass : AAlienBot::StaticClass();
	while (AlienPool.Num() < MaxLive)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AAlienBot* Bot = World->SpawnActor<AAlienBot>(ClassToSpawn, FTransform::Identity, Params);
		if (!Bot)
		{
			UE_LOG(LogNightShift, Error, TEXT("AArenaGameMode: failed to spawn AlienBot for pool."));
			break;
		}
		if (GameConfig)
		{
			Bot->GameConfig = GameConfig;
		}
		Bot->SoftDespawn();
		AlienPool.Add(Bot);
	}

	UE_LOG(LogNightShift, Log, TEXT("AArenaGameMode: alien pool size %d (MaxLiveAliens=%d)"),
		AlienPool.Num(), MaxLive);
}

void AArenaGameMode::RecordStartTransform()
{
	if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		StartTransform = Pawn->GetActorTransform();
		bHasStartTransform = true;
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (AActor* PlayerStart = UGameplayStatics::GetActorOfClass(World, APlayerStart::StaticClass()))
		{
			StartTransform = PlayerStart->GetActorTransform();
			bHasStartTransform = true;
		}
	}
}

void AArenaGameMode::ResetPlayerTransform()
{
	FTransform ResetXform = StartTransform;
	bool bHaveXform = bHasStartTransform;

	if (UWorld* World = GetWorld())
	{
		if (AActor* PlayerStart = UGameplayStatics::GetActorOfClass(World, APlayerStart::StaticClass()))
		{
			ResetXform = PlayerStart->GetActorTransform();
			bHaveXform = true;
		}
	}

	ANightShiftCharacter* Player = GetPlayerCharacter();
	if (!Player || !bHaveXform)
	{
		return;
	}

	Player->SetActorTransform(ResetXform, false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* C = Player->GetController())
	{
		C->SetControlRotation(ResetXform.GetRotation().Rotator());
	}
}

void AArenaGameMode::CreateAndBindHUD()
{
	if (!HUDWidgetClass)
	{
		if (!bLoggedMissingHUDClass)
		{
			bLoggedMissingHUDClass = true;
			UE_LOG(LogNightShift, Warning,
				TEXT("AArenaGameMode: HUDWidgetClass unset — match logic runs without HUD. Assign WBP in Editor."));
		}
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		UE_LOG(LogNightShift, Warning, TEXT("AArenaGameMode: no PlayerController — cannot create HUD."));
		return;
	}

	if (!HUDWidget)
	{
		HUDWidget = CreateWidget<UHUDWidget>(PC, HUDWidgetClass);
		if (!HUDWidget)
		{
			UE_LOG(LogNightShift, Error, TEXT("AArenaGameMode: CreateWidget failed for HUDWidgetClass."));
			return;
		}
		HUDWidget->AddToViewport();
	}

	EnsureHUDBound();
	HUDWidget->ShowStartPrompt();
}

void AArenaGameMode::EnsureHUDBound()
{
	if (!HUDWidget)
	{
		return;
	}

	ANightShiftCharacter* Character = GetPlayerCharacter();
	HUDWidget->BindToMatch(this, Character);

	if (Character && Character->Rifle)
	{
		Character->Rifle->OnHitConfirmed.RemoveDynamic(HUDWidget.Get(), &UHUDWidget::ShowHitMarker);
		Character->Rifle->OnHitConfirmed.AddDynamic(HUDWidget.Get(), &UHUDWidget::ShowHitMarker);
	}
}

void AArenaGameMode::UpdatePlayerInputMode()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	const bool bPrompting =
		MatchState == EArenaMatchState::WaitingToStart ||
		MatchState == EArenaMatchState::Lost ||
		MatchState == EArenaMatchState::Won;
	const bool bShowUI = bMatchPaused || bPrompting;

	PC->bShowMouseCursor = bShowUI;
	if (bShowUI)
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void AArenaGameMode::StartMatch()
{
	KillCount = 0;
	MatchTimeSeconds = 0.f;
	bMatchPaused = false;
	SetMatchState(EArenaMatchState::InProgress);
	FindOrCacheArena();
	EnsureHUDBound();

	if (CachedArena)
	{
		CachedArena->RefreshSpawnGather();
	}
	EnsureAlienPopulation();

	if (HUDWidget)
	{
		HUDWidget->ClearPrompt();
	}

	UpdatePlayerInputMode();
	UE_LOG(LogNightShift, Log, TEXT("Match started — %d live aliens."), GetLiveAlienCount());
}

void AArenaGameMode::RequestStartOrRestart()
{
	if (MatchState == EArenaMatchState::WaitingToStart)
	{
		StartMatch();
		return;
	}

	if (MatchState == EArenaMatchState::Lost || MatchState == EArenaMatchState::Won)
	{
		// One click: soft reset then immediately start (prefer over two-click prompt).
		SoftRestartInternal(false);
		StartMatch();
	}
}

void AArenaGameMode::RegisterKill(AActor* /*Victim*/)
{
	if (MatchState != EArenaMatchState::InProgress)
	{
		return;
	}
	++KillCount;
	CheckWinCondition();
	// Bot self-respawns after AlienRespawnSeconds via PerformRespawn → RespawnAlien.
	// EnsureAlienPopulation is a safety net if a pool slot was lost.
}

void AArenaGameMode::NotifyPlayerDied()
{
	SetMatchState(EArenaMatchState::Lost);
	bMatchPaused = false;

	if (HUDWidget)
	{
		HUDWidget->ShowDeathPrompt();
	}
	else
	{
		UE_LOG(LogNightShift, Log, TEXT("Player died — HUD missing (\"You died — click to restart\")."));
	}

	UpdatePlayerInputMode();
	UE_LOG(LogNightShift, Log, TEXT("Player died."));
}

void AArenaGameMode::SoftRestart()
{
	SoftRestartInternal(true);
}

void AArenaGameMode::SoftRestartInternal(bool bShowPromptIfWaiting)
{
	// DESIGN: soft reset without unloading level
	KillCount = 0;
	MatchTimeSeconds = 0.f;
	bMatchPaused = false;
	SetMatchState(EArenaMatchState::WaitingToStart);

	ResetPlayerTransform();

	if (ANightShiftCharacter* Player = GetPlayerCharacter())
	{
		Player->SoftResetPlayerState();
	}

	SoftRestartAlienPool();
	EnsureHUDBound();

	if (bShowPromptIfWaiting && HUDWidget)
	{
		HUDWidget->ShowStartPrompt();
	}

	UpdatePlayerInputMode();
	UE_LOG(LogNightShift, Log, TEXT("SoftRestart complete — alien pool reset (%d slots)."), AlienPool.Num());
}

void AArenaGameMode::SoftRestartAlienPool()
{
	FindOrCacheArena();
	if (CachedArena)
	{
		CachedArena->RefreshSpawnGather();
	}

	for (AAlienBot* Bot : AlienPool)
	{
		if (Bot)
		{
			Bot->SoftReset();
		}
	}

	// Rebuild if pool was empty (e.g. BeginPlay before arena existed).
	if (AlienPool.Num() == 0)
	{
		BuildAlienPool();
	}

	// Redistribute at farthest-from-player edge spawns so population is ready for StartMatch.
	EnsureAlienPopulation();
}

void AArenaGameMode::PauseMatch(bool bPause)
{
	bMatchPaused = bPause;
	UpdatePlayerInputMode();
	UE_LOG(LogNightShift, Log, TEXT("PauseMatch: %s"), bPause ? TEXT("paused") : TEXT("resumed"));
}

void AArenaGameMode::CheckWinCondition()
{
	const int32 Need = GameConfig ? GameConfig->KillsToWin : 25;
	if (KillCount >= Need)
	{
		SetMatchState(EArenaMatchState::Won);
		bMatchPaused = false;

		if (HUDWidget)
		{
			HUDWidget->ShowWin(MatchTimeSeconds);
		}

		UpdatePlayerInputMode();
		UE_LOG(LogNightShift, Log, TEXT("WIN — %d kills in %.2fs"), KillCount, MatchTimeSeconds);
	}
}

int32 AArenaGameMode::GetLiveAlienCount() const
{
	int32 Live = 0;
	for (const AAlienBot* Bot : AlienPool)
	{
		if (Bot && Bot->bIsAlive)
		{
			++Live;
		}
	}
	return Live;
}

bool AArenaGameMode::RespawnAlien(AAlienBot* Bot)
{
	if (!Bot)
	{
		return false;
	}
	FindOrCacheArena();
	if (!CachedArena)
	{
		return false;
	}

	const FTransform Spawn = CachedArena->GetFarthestSpawnFrom(GetPlayerLocation());
	if (ANightShiftCharacter* Player = GetPlayerCharacter())
	{
		Bot->SetTarget(Player);
	}
	if (GameConfig && !Bot->GameConfig)
	{
		Bot->GameConfig = GameConfig;
	}
	Bot->ActivateAtSpawn(Spawn);
	return true;
}

void AArenaGameMode::EnsureAlienPopulation()
{
	FindOrCacheArena();
	const int32 MaxLive = GameConfig ? GameConfig->MaxLiveAliens : 6;

	if (AlienPool.Num() == 0)
	{
		BuildAlienPool();
	}

	ANightShiftCharacter* Player = GetPlayerCharacter();
	const FVector PlayerLoc = GetPlayerLocation();

	int32 Live = GetLiveAlienCount();
	if (Live >= MaxLive || !CachedArena)
	{
		return;
	}

	for (AAlienBot* Bot : AlienPool)
	{
		if (Live >= MaxLive)
		{
			break;
		}
		if (!Bot || Bot->bIsAlive)
		{
			continue;
		}
		// Skip bots waiting on death→respawn timer (self-respawn owns that slot).
		if (Bot->IsRespawnPending())
		{
			continue;
		}

		const FTransform Spawn = CachedArena->GetFarthestSpawnFrom(PlayerLoc);
		if (Player)
		{
			Bot->SetTarget(Player);
		}
		if (GameConfig && !Bot->GameConfig)
		{
			Bot->GameConfig = GameConfig;
		}
		Bot->ActivateAtSpawn(Spawn);
		++Live;
	}
}

FVector AArenaGameMode::GetPlayerLocation() const
{
	if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		return P->GetActorLocation();
	}
	return FVector::ZeroVector;
}

ANightShiftCharacter* AArenaGameMode::GetPlayerCharacter() const
{
	return Cast<ANightShiftCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
}
