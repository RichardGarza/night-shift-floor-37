#include "ArenaGameMode.h"
#include "GameConfig.h"
#include "OfficeArena.h"
#include "AlienBot.h"
#include "NightShiftCharacter.h"
#include "HUDWidget.h"
#include "RifleComponent.h"
#include "ArenaCollision.h"
#include "NightShiftFloor37.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "FXPoolInterface.h"
#include "NightShiftSelfTest.h"
#include "Misc/CommandLine.h"
#include "TimerManager.h"

AArenaGameMode::AArenaGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	AlienBotClass = AAlienBot::StaticClass();
	// Code-first defaults: the game runs with no Blueprints. A BP subclass can still override both.
	DefaultPawnClass = ANightShiftCharacter::StaticClass();
	HUDWidgetClass = UHUDWidget::StaticClass();
}

AActor* AArenaGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (AActor* Found = Super::ChoosePlayerStart_Implementation(Player))
	{
		return Found;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	EnsureWorldActors();
	const FVector Origin = CachedArena ? CachedArena->GetActorLocation() : FVector::ZeroVector;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// 10 m west of the atrium, facing it (+X).
	APlayerStart* Start = World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), Origin + FVector(-1000.f, 0.f, 120.f), FRotator::ZeroRotator, Params);
	UE_LOG(LogNightShift, Log, TEXT("AArenaGameMode: no PlayerStart in map — spawned one beside the arena."));
	return Start;
}

void AArenaGameMode::EnsureWorldActors()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	FindOrCacheArena();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (!CachedArena)
	{
		CachedArena = World->SpawnActor<AOfficeArena>(AOfficeArena::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
		UE_LOG(LogNightShift, Log, TEXT("AArenaGameMode: spawned AOfficeArena (greybox) at origin."));
	}
	TActorIterator<AFXPoolManager> PoolIt(World);
	if (!PoolIt)
	{
		World->SpawnActor<AFXPoolManager>(AFXPoolManager::StaticClass(), FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator, Params);
		UE_LOG(LogNightShift, Log, TEXT("AArenaGameMode: spawned AFXPoolManager."));
	}
}

void AArenaGameMode::BeginPlay()
{
	Super::BeginPlay();
	EnsureWorldActors();
	ResolveAndPropagateGameConfig();
	FindOrCacheArena();
	BuildAlienPool();
	// Pool may build before pawn exists — propagate again for player/rifle/collision.
	ResolveAndPropagateGameConfig();
	RecordStartTransform();
	CreateAndBindHUD();
	SetMatchState(EArenaMatchState::WaitingToStart);
	UpdatePlayerInputMode();
	if (ANightShiftSelfTest::IsRequestedOnCommandLine())
	{
		GetWorld()->SpawnActor<ANightShiftSelfTest>();
	}
	// -NightShiftAutoStart: skip Click-to-play after 1.5 s (screenshots / smoke runs without a mouse).
	if (FParse::Param(FCommandLine::Get(), TEXT("NightShiftAutoStart")))
	{
		FTimerHandle AutoStartHandle;
		GetWorldTimerManager().SetTimer(AutoStartHandle, this, &AArenaGameMode::RequestStartOrRestart, 1.5f, false);
		UE_LOG(LogNightShift, Log, TEXT("AArenaGameMode: -NightShiftAutoStart — match starts in 1.5 s."));
	}
	UE_LOG(LogNightShift, Log, TEXT("AArenaGameMode::BeginPlay — waiting to start (win @ %d kills, pool %d, HUD %s, config %s)"),
		GameConfig ? GameConfig->KillsToWin : 25,
		AlienPool.Num(),
		HUDWidget ? TEXT("bound") : TEXT("missing"),
		GameConfig ? *GameConfig->GetName() : TEXT("null"));
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
	if (SpawnGraceRemaining > 0.f)
	{
		SpawnGraceRemaining = FMath::Max(0.f, SpawnGraceRemaining - DeltaSeconds);
		if (SpawnGraceRemaining <= 0.f)
		{
			const float FireDelay = GameConfig ? GameConfig->PostGraceAlienFireDelaySeconds : 1.5f;
			AlienFireLockRemaining = FMath::Max(0.f, FireDelay);
			UE_LOG(LogNightShift, Log, TEXT("Spawn grace ended — chase OK; fire locked %.1fs."), AlienFireLockRemaining);
		}
	}
	else if (AlienFireLockRemaining > 0.f)
	{
		AlienFireLockRemaining = FMath::Max(0.f, AlienFireLockRemaining - DeltaSeconds);
		if (AlienFireLockRemaining <= 0.f)
		{
			UE_LOG(LogNightShift, Log, TEXT("Post-grace fire lock ended — aliens may shoot."));
		}
	}
	EnsureAlienPopulation();
	EnforceArenaBounds();

	const int32 Tier = GetThreatTier();
	if (Tier != LoggedThreatTier)
	{
		LoggedThreatTier = Tier;
		UE_LOG(LogNightShift, Log, TEXT("Difficulty ramp — threat %d/5 (alpha %.2f): %d live aliens, accuracy %.0f%%, burst every %.1fs."),
			Tier, GetDifficultyAlpha(), GetTargetLiveAliens(), GetAlienAccuracy() * 100.f, GetAlienBurstInterval());
	}
}

float AArenaGameMode::GetDifficultyAlpha() const
{
	if (!GameConfig || !GameConfig->bDifficultyRamp)
	{
		return 1.f;
	}
	const float ByKills = KillCount / static_cast<float>(FMath::Max(GameConfig->RampKillsToMax, 1));
	const float ByTime = MatchTimeSeconds / FMath::Max(GameConfig->RampSecondsToMax, 1.f);
	return FMath::Clamp(FMath::Max(ByKills, ByTime), 0.f, 1.f);
}

int32 AArenaGameMode::GetTargetLiveAliens() const
{
	const int32 MaxLive = GameConfig ? GameConfig->MaxLiveAliens : 6;
	if (!GameConfig || !GameConfig->bDifficultyRamp)
	{
		return MaxLive;
	}
	const int32 Start = FMath::Clamp(GameConfig->RampStartLiveAliens, 1, MaxLive);
	// Floor so each extra alien arrives at an even kill/time step; alpha 1 always yields MaxLive.
	return Start + FMath::FloorToInt(GetDifficultyAlpha() * (MaxLive - Start) + 0.001f);
}

float AArenaGameMode::GetAlienAccuracy() const
{
	if (!GameConfig)
	{
		return 0.3f;
	}
	if (!GameConfig->bDifficultyRamp)
	{
		return GameConfig->AlienAccuracy;
	}
	return FMath::Lerp(GameConfig->RampStartAlienAccuracy, GameConfig->RampEndAlienAccuracy, GetDifficultyAlpha());
}

float AArenaGameMode::GetAlienBurstInterval() const
{
	if (!GameConfig)
	{
		return 1.5f;
	}
	if (!GameConfig->bDifficultyRamp)
	{
		return GameConfig->AlienBurstIntervalSeconds;
	}
	return FMath::Max(0.2f, FMath::Lerp(GameConfig->RampStartBurstIntervalSeconds, GameConfig->RampEndBurstIntervalSeconds, GetDifficultyAlpha()));
}

int32 AArenaGameMode::GetThreatTier() const
{
	return 1 + FMath::Clamp(FMath::FloorToInt(GetDifficultyAlpha() * 4.999f), 0, 4);
}

void AArenaGameMode::EnforceArenaBounds()
{
	// DESIGN: invisible ceiling / bounds clamp so nobody leaves the floor.
	if (!CachedArena)
	{
		return;
	}
	if (ANightShiftCharacter* Player = GetPlayerCharacter())
	{
		CachedArena->EnforceBoundsOnActor(Player);
	}
	for (AAlienBot* Bot : AlienPool)
	{
		if (Bot && Bot->bIsAlive)
		{
			CachedArena->EnforceBoundsOnActor(Bot);
		}
	}
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


void AArenaGameMode::ResolveAndPropagateGameConfig()
{
	GameConfig = UGameConfig::ResolveOrCreate(this, GameConfig);
	if (GameConfig)
	{
		GameConfig->ResolvePhase8LoadedMeshes(); // Sprint O — once per propagate
	}

	FindOrCacheArena();
	if (CachedArena && GameConfig)
	{
		CachedArena->GameConfig = GameConfig;
		CachedArena->ApplyConfiguredCoverMeshes();
		CachedArena->ApplyConfiguredOfficeDressMeshes();
		CachedArena->ApplyConfiguredServerRackMeshes();
		CachedArena->ApplyConfiguredFluorescentMeshes();
	}

	for (AAlienBot* Bot : AlienPool)
	{
		if (!Bot)
		{
			continue;
		}
		Bot->GameConfig = GameConfig;
		Bot->ApplyConfiguredMeshes();
		if (Bot->ArenaCollision)
		{
			Bot->ArenaCollision->GameConfig = GameConfig;
		}
	}

	if (ANightShiftCharacter* Player = GetPlayerCharacter())
	{
		Player->GameConfig = GameConfig;
		Player->ApplyResolvedGameConfig();
	}
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
	TActorIterator<AOfficeArena> It(World);
	if (It)
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
	TSubclassOf<AAlienBot> ClassToSpawn = AlienBotClass ? *AlienBotClass : AAlienBot::StaticClass();
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

void AArenaGameMode::SetMatchState(EArenaMatchState NewState)
{
	if (MatchState == NewState)
	{
		return;
	}
	MatchState = NewState;
	OnMatchStateChanged.Broadcast(NewState);
}

void AArenaGameMode::CreateAndBindHUD()
{
	if (HUDWidget)
	{
		EnsureHUDBound();
		return;
	}

	if (!HUDWidgetClass)
	{
		if (!bLoggedMissingHUDClass)
		{
			bLoggedMissingHUDClass = true;
			UE_LOG(LogNightShift, Warning,
				TEXT("AArenaGameMode: HUDWidgetClass unset — assign WBP_NightShiftHUD (or UHUDWidget BP) on the GameMode. Match logic still runs."));
		}
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		UE_LOG(LogNightShift, Warning, TEXT("AArenaGameMode: no PlayerController yet — HUD deferred."));
		return;
	}

	HUDWidget = CreateWidget<UHUDWidget>(PC, HUDWidgetClass);
	if (!HUDWidget)
	{
		UE_LOG(LogNightShift, Error, TEXT("AArenaGameMode: CreateWidget failed for HUDWidgetClass."));
		return;
	}

	HUDWidget->AddToViewport(100);
	EnsureHUDBound();
	HUDWidget->ShowStartPrompt();
}

void AArenaGameMode::EnsureHUDBound()
{
	if (!HUDWidget)
	{
		CreateAndBindHUD();
		if (!HUDWidget)
		{
			return;
		}
	}

	ANightShiftCharacter* Player = GetPlayerCharacter();
	HUDWidget->BindToMatch(this, Player);

	// After BindToMatch: rifle hits → HUD hit-marker
	if (Player && Player->Rifle)
	{
		Player->Rifle->OnHitConfirmed.RemoveAll(HUDWidget);
		Player->Rifle->OnHitConfirmed.AddDynamic(HUDWidget, &UHUDWidget::ShowHitMarker);
	}
}

void AArenaGameMode::RecordStartTransform()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TActorIterator<APlayerStart> StartIt(World);
	if (StartIt)
	{
		StartTransform = StartIt->GetActorTransform();
		bHasStartTransform = true;
		UE_LOG(LogNightShift, Log, TEXT("AArenaGameMode: start transform from APlayerStart %s"), *StartIt->GetName());
		return;
	}

	if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		StartTransform = Pawn->GetActorTransform();
		bHasStartTransform = true;
		UE_LOG(LogNightShift, Log, TEXT("AArenaGameMode: start transform from player pawn (no APlayerStart)."));
	}
}

void AArenaGameMode::ResetPlayerTransform()
{
	if (!bHasStartTransform)
	{
		RecordStartTransform();
	}
	if (!bHasStartTransform)
	{
		return;
	}

	if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Pawn->SetActorTransform(StartTransform, false, nullptr, ETeleportType::ResetPhysics);
		if (AController* C = Pawn->GetController())
		{
			C->SetControlRotation(StartTransform.Rotator());
		}
	}
}

void AArenaGameMode::UpdatePlayerInputMode()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	const bool bNeedUI =
		bMatchPaused
		|| MatchState == EArenaMatchState::WaitingToStart
		|| MatchState == EArenaMatchState::Lost
		|| MatchState == EArenaMatchState::Won;

	if (bNeedUI)
	{
		PC->bShowMouseCursor = true;
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		if (HUDWidget)
		{
			Mode.SetWidgetToFocus(HUDWidget->TakeWidget());
		}
		PC->SetInputMode(Mode);
	}
	else
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void AArenaGameMode::StartMatch()
{
	EnsureHUDBound();
	KillCount = 0;
	MatchTimeSeconds = 0.f;
	bMatchPaused = false;
	SetMatchState(EArenaMatchState::InProgress);
	FindOrCacheArena();
	if (CachedArena)
	{
		CachedArena->RefreshSpawnGather();
	}
	BeginSpawnGraceAndSafeStart();
	LoggedThreatTier = 0;
	EnsureAlienPopulation();
	ApplySaferStartSpacing(); // re-push any bot that landed too close
	OrientPlayerTowardStartFocus(); // Sprint M — silhouette in frame without closing gap
	if (HUDWidget)
	{
		HUDWidget->ClearPrompt();
	}
	UpdatePlayerInputMode();
	UE_LOG(LogNightShift, Log, TEXT("Match started — %d live aliens (target %d, ramp %s), grace %.1fs."),
		GetLiveAlienCount(), GetTargetLiveAliens(),
		(GameConfig && GameConfig->bDifficultyRamp) ? TEXT("on") : TEXT("off"), SpawnGraceRemaining);
}

void AArenaGameMode::RequestStartOrRestart()
{
	EnsureHUDBound();

	if (MatchState == EArenaMatchState::WaitingToStart)
	{
		StartMatch();
		return;
	}

	if (MatchState == EArenaMatchState::Lost || MatchState == EArenaMatchState::Won)
	{
		// One-click restart: skip "Click to play" gate
		SoftRestartInternal(/*bShowPromptIfWaiting=*/false);
		StartMatch();
		return;
	}

	// InProgress — ignore click (Esc pause owns unlock)
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
	if (MatchState != EArenaMatchState::InProgress)
	{
		return;
	}
	SetMatchState(EArenaMatchState::Lost);
	if (HUDWidget)
	{
		HUDWidget->ShowDeathPrompt();
	}
	UpdatePlayerInputMode();
	UE_LOG(LogNightShift, Log, TEXT("Player died."));
}

void AArenaGameMode::SoftRestart()
{
	// Public soft reset → WaitingToStart + "Click to play"
	SoftRestartInternal(/*bShowPromptIfWaiting=*/true);
}

void AArenaGameMode::SoftRestartInternal(bool bShowPromptIfWaiting)
{
	// DESIGN: soft reset without unloading level
	KillCount = 0;
	MatchTimeSeconds = 0.f;
	bMatchPaused = false;
	SpawnGraceRemaining = 0.f;
	AlienFireLockRemaining = 0.f;
	SetMatchState(EArenaMatchState::WaitingToStart);

	ResetPlayerTransform();

	if (ANightShiftCharacter* Player = GetPlayerCharacter())
	{
		Player->SoftResetPlayerState();
	}

	SoftRestartAlienPool();

	if (HUDWidget)
	{
		if (bShowPromptIfWaiting)
		{
			HUDWidget->ShowStartPrompt();
		}
		else
		{
			HUDWidget->ClearPrompt();
		}
	}
	UpdatePlayerInputMode();

	UE_LOG(LogNightShift, Log, TEXT("SoftRestartInternal(prompt=%s) — alien pool reset (%d slots)."),
		bShowPromptIfWaiting ? TEXT("true") : TEXT("false"), AlienPool.Num());
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
			Bot->SoftReset(); // SoftDespawn — stay inactive until StartMatch
		}
	}

	// Rebuild if pool was empty (e.g. BeginPlay before arena existed).
	if (AlienPool.Num() == 0)
	{
		BuildAlienPool();
	}

	// Do NOT EnsureAlienPopulation here — WaitingToStart must not leave chasing bots.
	// StartMatch / InProgress tick calls EnsureAlienPopulation.
}

void AArenaGameMode::PauseMatch(bool bPause)
{
	// Only pause during an active match; always allow clear when already paused.
	if (bPause && MatchState != EArenaMatchState::InProgress && !bMatchPaused)
	{
		return;
	}
	bMatchPaused = bPause;
	// Esc → pause / unlock (DESIGN input)
	UpdatePlayerInputMode();
	UE_LOG(LogNightShift, Log, TEXT("PauseMatch: %s"), bMatchPaused ? TEXT("paused") : TEXT("resumed"));
}

void AArenaGameMode::CheckWinCondition()
{
	const int32 Need = GameConfig ? GameConfig->KillsToWin : 25;
	if (KillCount >= Need)
	{
		SetMatchState(EArenaMatchState::Won);
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



void AArenaGameMode::OrientPlayerTowardStartFocus()
{
	// Sprint M — safer-start leaves aliens on far edges; yaw player/OTS camera so one silhouette reads.
	// Does NOT move anyone — MinStartSeparation + spawn grace unchanged.
	ANightShiftCharacter* Player = GetPlayerCharacter();
	if (!Player)
	{
		return;
	}

	const FVector Eye = Player->GetActorLocation() + FVector(0.f, 0.f, 60.f);
	FVector Focus = CachedArena ? CachedArena->GetActorLocation() : FVector::ZeroVector;
	float BestDistSq = TNumericLimits<float>::Max();
	AAlienBot* Nearest = nullptr;

	for (AAlienBot* Bot : AlienPool)
	{
		if (!Bot || !Bot->bIsAlive)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Bot->GetActorLocation(), Player->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Nearest = Bot;
		}
	}

	if (Nearest)
	{
		Focus = Nearest->GetActorLocation() + FVector(0.f, 0.f, 40.f);
	}
	else if (!CachedArena)
	{
		UE_LOG(LogNightShift, Verbose, TEXT("OrientPlayerTowardStartFocus: no alien/arena focus."));
		return;
	}

	FVector Delta = Focus - Eye;
	Delta.Z = 0.f; // yaw-only so OTS pitch stays comfortable
	if (Delta.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FRotator YawOnly = Delta.Rotation();
	FRotator NewControl = YawOnly;
	NewControl.Pitch = -8.f; // slight look-down toward floor/aliens mid-arena
	NewControl.Roll = 0.f;

	if (AController* C = Player->GetController())
	{
		C->SetControlRotation(NewControl);
	}
	Player->SetActorRotation(FRotator(0.f, NewControl.Yaw, 0.f));

	const float FocusDistM = FVector::Dist(Player->GetActorLocation(), Focus) * 0.01f;
	UE_LOG(LogNightShift, Log, TEXT("OrientPlayerTowardStartFocus — yaw %.0f toward %s (dist≈%.0fm)."),
		NewControl.Yaw,
		Nearest ? *Nearest->GetName() : TEXT("atrium"),
		FocusDistM);
}

void AArenaGameMode::BeginSpawnGraceAndSafeStart()
{
	const float Grace = GameConfig ? GameConfig->SpawnGraceSeconds : 7.f;
	SpawnGraceRemaining = FMath::Max(0.f, Grace);
	AlienFireLockRemaining = 0.f; // fire stays blocked via grace until PostGrace delay arms

	FindOrCacheArena();
	ANightShiftCharacter* Player = GetPlayerCharacter();
	if (!CachedArena || !Player)
	{
		UE_LOG(LogNightShift, Warning, TEXT("BeginSpawnGraceAndSafeStart: missing arena/player (grace=%.1fs)."), SpawnGraceRemaining);
		return;
	}

	// Place player on an edge spawn farthest from arena center first so aliens (farthest-from-player)
	// land on the opposite side of the floor.
	const FVector ArenaOrigin = CachedArena->GetActorLocation();
	const FTransform SafePlayerXf = CachedArena->GetFarthestSpawnFrom(ArenaOrigin);
	FVector SafeLoc = SafePlayerXf.GetLocation();
	SafeLoc.Z = Player->GetActorLocation().Z; // keep capsule height
	Player->SetActorLocation(SafeLoc, false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* C = Player->GetController())
	{
		C->SetControlRotation(SafePlayerXf.GetRotation().Rotator());
	}
	UE_LOG(LogNightShift, Log, TEXT("Safer start: player → edge spawn, grace %.1fs."), SpawnGraceRemaining);
}

void AArenaGameMode::ApplySaferStartSpacing()
{
	FindOrCacheArena();
	ANightShiftCharacter* Player = GetPlayerCharacter();
	if (!CachedArena || !Player)
	{
		return;
	}
	const float MinM = GameConfig ? GameConfig->MinStartSeparationMeters : 18.f;
	const float MinCmSq = (MinM * 100.f) * (MinM * 100.f);
	const FVector PlayerLoc = Player->GetActorLocation();

	TArray<int32> Used;
	// Sprint L — seed Used with spawns already occupied by far-enough bots so relocate cannot double-book.
	for (AAlienBot* Bot : AlienPool)
	{
		if (!Bot || !Bot->bIsAlive)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Bot->GetActorLocation(), PlayerLoc);
		if (DistSq < MinCmSq)
		{
			continue; // will relocate below
		}
		const int32 Occ = CachedArena->FindNearestSpawnIndex(Bot->GetActorLocation());
		if (Occ >= 0)
		{
			Used.AddUnique(Occ);
		}
	}

	for (AAlienBot* Bot : AlienPool)
	{
		if (!Bot || !Bot->bIsAlive)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Bot->GetActorLocation(), PlayerLoc);
		if (DistSq >= MinCmSq)
		{
			continue;
		}
		int32 Idx = -1;
		const FTransform Spawn = CachedArena->GetFarthestUnusedSpawnFrom(PlayerLoc, Used, Idx);
		if (Idx >= 0)
		{
			Used.AddUnique(Idx);
		}
		Bot->ActivateAtSpawn(Spawn);
		UE_LOG(LogNightShift, Log, TEXT("Safer start: relocated close alien %s (min %.0fm)."), *Bot->GetName(), MinM);
	}
}

void AArenaGameMode::EnsureAlienPopulation()
{
	// Only populate while a match is live — SoftRestart leaves pool despawned in WaitingToStart.
	if (MatchState != EArenaMatchState::InProgress)
	{
		return;
	}

	FindOrCacheArena();
	// Sprint Y — the ramp decides how many of the pool are live; the pool itself stays MaxLiveAliens.
	const int32 MaxLive = GetTargetLiveAliens();

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

	// Spread this pass across distinct spawn points; GetFarthestSpawnFrom alone is deterministic
	// and would stack every bot on one corner.
	TArray<int32> UsedSpawnIndices;
	UsedSpawnIndices.Reserve(MaxLive);

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

		int32 SpawnIndex = -1;
		const FTransform Spawn = CachedArena->GetFarthestUnusedSpawnFrom(PlayerLoc, UsedSpawnIndices, SpawnIndex);
		if (SpawnIndex >= 0)
		{
			UsedSpawnIndices.Add(SpawnIndex);
		}
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
