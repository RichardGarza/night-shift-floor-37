#include "NightShiftSelfTest.h"
#include "ArenaGameMode.h"
#include "NightShiftCharacter.h"
#include "AlienBot.h"
#include "RifleComponent.h"
#include "GameConfig.h"
#include "NightShiftFloor37.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "HAL/PlatformMisc.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ANightShiftSelfTest::ANightShiftSelfTest()
{
	PrimaryActorTick.bCanEverTick = true;
}

bool ANightShiftSelfTest::IsRequestedOnCommandLine()
{
	return FParse::Param(FCommandLine::Get(), TEXT("NightShiftSelfTest"));
}

void ANightShiftSelfTest::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogNightShift, Display, TEXT("SELFTEST: armed"));
}

void ANightShiftSelfTest::Enter(EStep Next)
{
	Step = Next;
	StepTime = 0.f;
	bStepStarted = false;
}

void ANightShiftSelfTest::Check(bool bCondition, const FString& What)
{
	if (bCondition)
	{
		++Passed;
		UE_LOG(LogNightShift, Display, TEXT("SELFTEST PASS: %s"), *What);
	}
	else
	{
		Fail(What);
	}
}

void ANightShiftSelfTest::Fail(const FString& What)
{
	++Failed;
	Failures.Add(What);
	UE_LOG(LogNightShift, Error, TEXT("SELFTEST FAIL: %s"), *What);
}

void ANightShiftSelfTest::GatherBots(TArray<AAlienBot*>& Out) const
{
	Out.Reset();
	for (TActorIterator<AAlienBot> It(GetWorld()); It; ++It)
	{
		Out.Add(*It);
	}
}

int32 ANightShiftSelfTest::LiveBots() const
{
	int32 Live = 0;
	for (TActorIterator<AAlienBot> It(GetWorld()); It; ++It)
	{
		if (It->bIsAlive)
		{
			++Live;
		}
	}
	return Live;
}

void ANightShiftSelfTest::AimAt(const FVector& Target)
{
	if (!Player.IsValid())
	{
		return;
	}
	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		PC->SetControlRotation((Target - Player->GetAimOrigin()).Rotation());
	}
}

void ANightShiftSelfTest::Finish()
{
	UE_LOG(LogNightShift, Display, TEXT("NIGHTSHIFT SELFTEST: %d passed, %d failed"), Passed, Failed);
	for (const FString& F : Failures)
	{
		UE_LOG(LogNightShift, Display, TEXT("  failed: %s"), *F);
	}
}

void ANightShiftSelfTest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	StepTime += DeltaSeconds;

	if (!GM.IsValid())
	{
		GM = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	}
	if (!Player.IsValid())
	{
		Player = Cast<ANightShiftCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	}
	if (Step != EStep::Boot && (!GM.IsValid() || !Player.IsValid()))
	{
		return;
	}

	const float MaxHP = (GM.IsValid() && GM->GameConfig) ? GM->GameConfig->PlayerMaxHealth : 100.f;
	const int32 MaxLive = (GM.IsValid() && GM->GameConfig) ? GM->GameConfig->MaxLiveAliens : 6;
	const int32 KillsToWin = (GM.IsValid() && GM->GameConfig) ? GM->GameConfig->KillsToWin : 25;

	// Six bots converge on a stationary player; keep it alive until the Death step is deliberate.
	if (Player.IsValid() && Step < EStep::Death && Player->Health < MaxHP)
	{
		if (Step == EStep::GraceWait)
		{
			++HitsDuringGraceWait; // Sprint V: nothing should land while grace / fire lock holds
		}
		Player->Health = MaxHP;
	}

	const bool bFirst = !bStepStarted;
	bStepStarted = true;

	switch (Step)
	{
	case EStep::Boot:
		if (GM.IsValid() && Player.IsValid() && StepTime > 1.f)
		{
			Check(GM->MatchState == EArenaMatchState::WaitingToStart, TEXT("boots into WaitingToStart"));
			GM->RequestStartOrRestart();
			Enter(EStep::Start);
		}
		else if (StepTime > 10.f)
		{
			Fail(TEXT("GameMode / player never appeared"));
			Enter(EStep::Done);
		}
		break;

	case EStep::Start:
		if (StepTime > 0.5f)
		{
			Check(GM->MatchState == EArenaMatchState::InProgress, TEXT("click-to-play starts the match"));
			Enter(EStep::Spawns);
		}
		break;

	case EStep::Spawns:
		if (StepTime > 0.5f)
		{
			TArray<AAlienBot*> Bots;
			GatherBots(Bots);
			TArray<FVector> Pos;
			bool bAboveFloor = true;
			float NearestD = TNumericLimits<float>::Max();
			float FarthestD = -1.f;
			for (AAlienBot* B : Bots)
			{
				if (!B->bIsAlive)
				{
					continue;
				}
				const FVector L = B->GetActorLocation();
				Pos.Add(L);
				bAboveFloor &= (L.Z > 0.f);
				const float D = FVector::Dist2D(L, Player->GetActorLocation());
				if (D < NearestD) { NearestD = D; TargetBot = B; }
				if (D > FarthestD) { FarthestD = D; WatchBot = B; }
			}
			int32 Distinct = 0;
			for (int32 i = 0; i < Pos.Num(); ++i)
			{
				bool bDup = false;
				for (int32 j = 0; j < i; ++j)
				{
					if (FVector::Dist2D(Pos[i], Pos[j]) < 300.f) { bDup = true; break; }
				}
				if (!bDup) { ++Distinct; }
			}
			// Sprint Y — the ramp starts the match below MaxLiveAliens; the population keeper tracks the target.
			const int32 WantLive = GM->GetTargetLiveAliens();
			Check(Pos.Num() == WantLive, FString::Printf(TEXT("%d aliens live at start (want %d)"), Pos.Num(), WantLive));
			Check(Distinct >= FMath::Min(WantLive, 5), FString::Printf(TEXT("spawn spread: %d distinct points for %d aliens"), Distinct, Pos.Num()));
			Check(bAboveFloor, TEXT("aliens spawn above the floor"));
			// Sprint AF — audio wiring.
			Check(GM->GameConfig && GM->GameConfig->CachedSoundRifleFire != nullptr, TEXT("rifle shot sound resolved"));
			Check(GM->GetAmbientLoop() != nullptr && GM->GetAmbientLoop()->IsPlaying(), TEXT("ambient hum loop is playing"));
			const bool bRamp = GM->GameConfig && GM->GameConfig->bDifficultyRamp;
			Check(!bRamp || WantLive < MaxLive, FString::Printf(TEXT("difficulty ramp starts easy (%d of %d aliens, threat %d/5)"), WantLive, MaxLive, GM->GetThreatTier()));
			Check(!bRamp || GM->GetAlienAccuracy() < (GM->GameConfig ? GM->GameConfig->AlienAccuracy : 0.3f),
				FString::Printf(TEXT("alien accuracy starts below DESIGN (%.0f%%)"), GM->GetAlienAccuracy() * 100.f));
			Enter(EStep::Aim);
		}
		break;

	case EStep::Aim:
		if (!TargetBot.IsValid())
		{
			Fail(TEXT("no target alien available"));
			Enter(EStep::GraceWait);
			break;
		}
		if (bFirst)
		{
			FVector Dest = Player->GetActorLocation() + Player->GetActorForwardVector() * 600.f;
			Dest.Z = Player->GetActorLocation().Z;
			TargetBot->SetActorLocation(Dest, false, nullptr, ETeleportType::TeleportPhysics);
		}
		AimAt(TargetBot->GetActorLocation());
		if (StepTime > 0.3f)
		{
			KillsAtFireStart = GM->KillCount;
			Player->StartSprint(); // Sprint Y — firing must drop sprint to walk speed
			Player->StartFire();   // → Rifle->Fire() + sprint suppression
			Enter(EStep::Fire);
		}
		break;

	case EStep::Fire:
		AimAt(TargetBot->GetActorLocation());
		if (TargetBot->BodyHitCount + TargetBot->HeadHitCount > 0 || !TargetBot->bIsAlive)
		{
			Check(true, TEXT("rifle hitscan lands on the alien capsule"));
			const float Walk = GM->GameConfig ? GM->GameConfig->WalkSpeed : 600.f;
			const float MaxSpeed = Player->GetCharacterMovement() ? Player->GetCharacterMovement()->MaxWalkSpeed : 0.f;
			Check(Player->bWantsSprint && MaxSpeed <= Walk + 1.f,
				FString::Printf(TEXT("firing drops sprint to walk speed (%.0f cm/s)"), MaxSpeed));
			Player->StopSprint();
			Enter(EStep::Kill);
		}
		else if (StepTime > 2.f)
		{
			Fail(TEXT("no rifle hits registered on the alien within 2 s"));
			if (Player->Rifle) { Player->Rifle->StopFire(); }
			Enter(EStep::GraceWait);
		}
		break;

	case EStep::Kill:
		AimAt(TargetBot->GetActorLocation());
		if (!TargetBot->bIsAlive)
		{
			if (Player->Rifle) { Player->Rifle->StopFire(); }
			Check(GM->KillCount == KillsAtFireStart + 1, FString::Printf(TEXT("kill counted (%d → %d)"), KillsAtFireStart, GM->KillCount));
			DeathPos = TargetBot->GetActorLocation();
			Enter(EStep::Respawn);
		}
		else if (StepTime > 4.f)
		{
			Fail(FString::Printf(TEXT("alien did not die within 4 s of sustained fire (body %d, head %d)"), TargetBot->BodyHitCount, TargetBot->HeadHitCount));
			if (Player->Rifle) { Player->Rifle->StopFire(); }
			Enter(EStep::GraceWait);
		}
		break;

	case EStep::Respawn:
		if (TargetBot->bIsAlive)
		{
			const FVector L = TargetBot->GetActorLocation();
			Check(StepTime >= 2.5f, FString::Printf(TEXT("respawn waited ~3 s (%.1f s)"), StepTime));
			Check(FVector::Dist2D(L, DeathPos) > 500.f, TEXT("respawned away from the death spot"));
			Check(FMath::Max(FMath::Abs(L.X), FMath::Abs(L.Y)) > 2000.f, FString::Printf(TEXT("respawned at an edge spawn (%.0f, %.0f)"), L.X, L.Y));
			// Sprint W/X — visual wiring (soft assets must resolve in the runnable project).
			Check(Player->bUsingSkeletalBody, TEXT("player uses the skeletal mannequin, not the greybox cylinder"));
			Check(Player->RifleMeshComp && Player->RifleMeshComp->GetStaticMesh() != nullptr && Player->RifleMeshComp->IsVisible(), TEXT("rifle prop attached and visible"));
			Check(TargetBot->bSkeletalActive, TEXT("alien uses the skeletal Quaternius body"));
			Check(TargetBot->GetMesh() && TargetBot->GetMesh()->IsPlaying(), TEXT("alien skeletal animation is playing"));
			Check(Player->GetMesh() && Player->GetMesh()->IsPlaying(), TEXT("player skeletal animation is playing"));
			Enter(GM->IsWaveProgression() ? EStep::WaveClear : EStep::GraceWait);
		}
		else if (StepTime > 6.f)
		{
			Fail(TEXT("alien never respawned"));
			Enter(GM->IsWaveProgression() ? EStep::WaveClear : EStep::GraceWait);
		}
		break;

	case EStep::WaveClear:
		// Sprint Z — meeting the wave quota empties the floor, restocks, and starts the breather.
		if (bFirst)
		{
			Check(GM->CurrentWave == 1, TEXT("match opens on wave 1"));
			AccuracyAtWave1 = GM->GetAlienAccuracy();
			if (Player->Rifle) { Player->Rifle->MagAmmo = 3; }
			GM->WaveKills = GM->GetWaveKillQuota() - 1;
			GM->RegisterKill(nullptr);
		}
		if (StepTime > 0.3f)
		{
			int32 Mag = 0, Reserve = 0;
			if (Player->Rifle) { Player->Rifle->GetAmmo(Mag, Reserve); }
			const int32 MagSize = GM->GameConfig ? GM->GameConfig->MagSize : 30;
			Check(GM->IsInWaveBreak(), TEXT("wave quota met → breather"));
			Check(LiveBots() == 0, FString::Printf(TEXT("floor emptied for the breather (%d live)"), LiveBots()));
			Check(Mag == MagSize, FString::Printf(TEXT("ammo restocked on wave clear (%d / %d)"), Mag, Reserve));
			Check(GM->IsAlienFireLocked(), TEXT("aliens cannot fire during the breather"));
			Enter(EStep::WaveNext);
		}
		break;

	case EStep::WaveNext:
		if (!GM->IsInWaveBreak() && StepTime > 0.5f)
		{
			Check(GM->CurrentWave == 2, FString::Printf(TEXT("wave 2 starts after the breather (wave %d)"), GM->CurrentWave));
			Check(GM->WaveKills == 0, TEXT("wave kill counter reset"));
			Check(GM->GetTargetLiveAliens() == 3 && LiveBots() == 3,
				FString::Printf(TEXT("wave 2 fields more aliens (target %d, live %d)"), GM->GetTargetLiveAliens(), LiveBots()));
			Check(GM->GetAlienAccuracy() > AccuracyAtWave1,
				FString::Printf(TEXT("wave 2 aliens shoot straighter (%.0f%% → %.0f%%)"), AccuracyAtWave1 * 100.f, GM->GetAlienAccuracy() * 100.f));
			Enter(EStep::GraceWait);
		}
		else if (StepTime > 12.f)
		{
			Fail(TEXT("breather never ended"));
			Enter(EStep::GraceWait);
		}
		break;

	case EStep::GraceWait:
		// Sprint V — grace (aliens idle, player immune) then fire lock (chase OK, no shots).
		// Pause / death checks below need live, armed aliens, so wait for both to elapse.
		if (!GM->IsAlienFireLocked())
		{
			const float GraceS = GM->GameConfig ? GM->GameConfig->SpawnGraceSeconds : 7.f;
			const float LockS = GM->GameConfig ? GM->GameConfig->PostGraceAlienFireDelaySeconds : 1.5f;
			Check(GM->MatchTimeSeconds >= GraceS + LockS - 0.1f,
				FString::Printf(TEXT("spawn grace + fire lock elapsed (match t=%.1f s, want >= %.1f s)"), GM->MatchTimeSeconds, GraceS + LockS));
			Check(LiveBots() > 0, FString::Printf(TEXT("aliens are on the floor after the locks (%d live)"), LiveBots()));
			Check(HitsDuringGraceWait == 0,
				FString::Printf(TEXT("no alien damage landed before the fire lock ended (%d hits)"), HitsDuringGraceWait));
			Enter(EStep::PauseHold);
		}
		else if (StepTime > 20.f)
		{
			Fail(TEXT("spawn grace / fire lock never released within 20 s"));
			Enter(EStep::PauseHold);
		}
		break;

	case EStep::PauseHold:
		if (bFirst)
		{
			if (!WatchBot.IsValid() || !WatchBot->bIsAlive)
			{
				TArray<AAlienBot*> Bots; GatherBots(Bots);
				for (AAlienBot* B : Bots) { if (B->bIsAlive) { WatchBot = B; break; } }
			}
			GM->PauseMatch(true);
			WatchPos = WatchBot.IsValid() ? WatchBot->GetActorLocation() : FVector::ZeroVector;
			PausedTime = GM->MatchTimeSeconds;
		}
		if (StepTime > 1.f)
		{
			Check(GM->IsMatchPaused(), TEXT("Esc pauses the match"));
			const float Moved = WatchBot.IsValid() ? FVector::Dist(WatchBot->GetActorLocation(), WatchPos) : 0.f;
			Check(Moved < 5.f, FString::Printf(TEXT("alien frozen while paused (moved %.1f cm)"), Moved));
			Check(FMath::IsNearlyEqual(GM->MatchTimeSeconds, PausedTime, 0.01f), TEXT("match timer frozen while paused"));
			GM->PauseMatch(false);
			WatchPos = WatchBot.IsValid() ? WatchBot->GetActorLocation() : FVector::ZeroVector;
			Enter(EStep::PauseResume);
		}
		break;

	case EStep::PauseResume:
		if (StepTime > 1.f)
		{
			Check(!GM->IsMatchPaused(), TEXT("resume clears pause"));
			Check(GM->MatchTimeSeconds > PausedTime + 0.5f, TEXT("match timer runs after resume"));
			const float Moved = WatchBot.IsValid() ? FVector::Dist(WatchBot->GetActorLocation(), WatchPos) : 0.f;
			Check(Moved > 5.f, FString::Printf(TEXT("alien moves again after resume (%.0f cm)"), Moved));
			Enter(EStep::Bounds);
		}
		break;

	case EStep::Bounds:
		if (bFirst)
		{
			Player->SetActorLocation(FVector(4200.f, 0.f, 150.f), false, nullptr, ETeleportType::TeleportPhysics);
		}
		if (StepTime > 0.5f)
		{
			const FVector L = Player->GetActorLocation();
			Check(FMath::Abs(L.X) <= 2550.f && FMath::Abs(L.Y) <= 2550.f, FString::Printf(TEXT("player clamped back inside the arena (x=%.0f)"), L.X));
			Player->SetActorLocation(FVector(-1000.f, 0.f, 120.f), false, nullptr, ETeleportType::TeleportPhysics);
			Enter(EStep::Death);
		}
		break;

	case EStep::Death:
		if (bFirst)
		{
			Player->TakeDamage(10000.f, FDamageEvent(), nullptr, nullptr);
		}
		if (StepTime > 0.5f)
		{
			Check(!Player->IsAlive(), TEXT("lethal damage kills the player"));
			Check(GM->MatchState == EArenaMatchState::Lost, TEXT("player death → Lost"));
			Enter(EStep::Restart);
		}
		break;

	case EStep::Restart:
		if (bFirst)
		{
			GM->RequestStartOrRestart();
		}
		if (StepTime > 0.7f)
		{
			Check(GM->MatchState == EArenaMatchState::InProgress, TEXT("click after death restarts in place"));
			Check(Player->IsAlive() && Player->Health >= MaxHP - 1.f, FString::Printf(TEXT("HP reset on restart (%.0f)"), Player->Health));
			Check(GM->KillCount == 0, TEXT("kills reset on restart"));
			Check(LiveBots() == GM->GetTargetLiveAliens(), FString::Printf(TEXT("aliens repopulated on restart (%d, target %d)"), LiveBots(), GM->GetTargetLiveAliens()));
			Check(GM->CurrentWave == 1 && GM->WaveKills == 0, TEXT("wave counter reset on restart"));
			Enter(EStep::Win);
		}
		break;

	case EStep::Win:
		if (bFirst)
		{
			if (GM->IsWaveProgression())
			{
				// Jump to the last wave, one kill short of its quota; the population keeper follows the wave.
				GM->CurrentWave = FMath::Max(1, GM->GetWavesToWin());
				GM->WaveKills = GM->GetWaveKillQuota() - 1;
				GM->KillCount = 59;
			}
			else
			{
				GM->KillCount = KillsToWin - 1;
			}
			GM->RegisterKill(nullptr);
		}
		if (StepTime > 0.3f)
		{
			if (GM->IsWaveProgression())
			{
				Check(GM->HasWon(), FString::Printf(TEXT("clearing wave %d → Won"), GM->GetWavesToWin()));
				Check(GM->GetThreatTier() == 5 && GM->GetAlienAccuracy() > AccuracyAtWave1 + 0.1f,
					FString::Printf(TEXT("final wave is full pressure (threat %d/5, accuracy %.0f%%)"), GM->GetThreatTier(), GM->GetAlienAccuracy() * 100.f));
				Check(GM->PickVariantForSpawn() != EAlienVariant::Grunt, TEXT("variants are in the spawn mix on the final wave"));
			}
			else
			{
				Check(GM->HasWon(), FString::Printf(TEXT("kill %d → Won"), KillsToWin));
				Check(GM->GetTargetLiveAliens() == MaxLive && GM->GetThreatTier() == 5,
					FString::Printf(TEXT("difficulty ramp is maxed by the win (%d aliens, threat %d/5)"), GM->GetTargetLiveAliens(), GM->GetThreatTier()));
			}
			Enter(EStep::Done);
		}
		break;

	case EStep::Done:
		Finish();
		Enter(EStep::Exit);
		break;

	case EStep::Exit:
		if (StepTime > 0.5f)
		{
			FPlatformMisc::RequestExit(false);
			SetActorTickEnabled(false);
		}
		break;
	}
}
