// Night Shift — Floor 37 | In-game self-test: drives the match loop and logs PASS/FAIL
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NightShiftSelfTest.generated.h"

class AArenaGameMode;
class ANightShiftCharacter;
class AAlienBot;

/**
 * Spawned by AArenaGameMode when the process was launched with -NightShiftSelfTest.
 * Starts the match, teleports an alien in front of the player and fires until it dies,
 * waits for the respawn, pauses, clamps bounds, kills the player, restarts, and wins.
 * Every check logs "SELFTEST PASS/FAIL: ..." and the run ends with a summary + RequestExit.
 *
 *   UnrealEditor <proj>.uproject /Game/Maps/Floor37 -game -nullrhi -unattended -NightShiftSelfTest -abslog=<file>
 */
UCLASS()
class NIGHTSHIFTFLOOR37_API ANightShiftSelfTest : public AActor
{
	GENERATED_BODY()
public:
	ANightShiftSelfTest();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	static bool IsRequestedOnCommandLine();

protected:
	enum class EStep : uint8
	{
		Boot, Start, Spawns, Aim, Fire, Kill, Respawn, PauseHold, PauseResume, Bounds, Death, Restart, Win, Done, Exit
	};

	EStep Step = EStep::Boot;
	float StepTime = 0.f;
	bool bStepStarted = false;
	int32 Passed = 0;
	int32 Failed = 0;
	TArray<FString> Failures;

	TWeakObjectPtr<AArenaGameMode> GM;
	TWeakObjectPtr<ANightShiftCharacter> Player;
	TWeakObjectPtr<AAlienBot> TargetBot;
	TWeakObjectPtr<AAlienBot> WatchBot;
	FVector WatchPos = FVector::ZeroVector;
	FVector DeathPos = FVector::ZeroVector;
	float PausedTime = 0.f;
	int32 KillsAtFireStart = 0;

	void Enter(EStep Next);
	void Check(bool bCondition, const FString& What);
	void Fail(const FString& What);
	void GatherBots(TArray<AAlienBot*>& Out) const;
	int32 LiveBots() const;
	void AimAt(const FVector& Target);
	void Finish();
};
