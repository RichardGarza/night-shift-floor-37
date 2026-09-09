// Night Shift — Floor 37 | Sprint AG: hands-off autopilot demo (attract mode)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NightShiftDemoPilot.generated.h"

class AArenaGameMode;
class ANightShiftCharacter;
class AAlienBot;

/**
 * Drives the player for ~60 s with no input: picks the nearest visible alien, tracks its head with a
 * smoothed (slightly noisy) aim, fires when aligned, keeps a 7–14 m band from the closest alien,
 * strafes with periodic direction flips, stays inside the arena, reloads on empty. Spawned by
 * AArenaGameMode when -NightShiftDemo is on the command line or the start prompt idles for
 * UGameConfig::DemoIdleSeconds. Any click ends the demo and hands the match to the human.
 */
UCLASS()
class NIGHTSHIFTFLOOR37_API ANightShiftDemoPilot : public AActor
{
	GENERATED_BODY()
public:
	ANightShiftDemoPilot();
	virtual void Tick(float DeltaSeconds) override;

	/** Seconds the demo has been running. */
	float Elapsed = 0.f;
	/** Kills the pilot has landed (for the end-of-demo log line). */
	int32 KillsAtStart = 0;

protected:
	AAlienBot* PickTarget(ANightShiftCharacter* Player, AArenaGameMode* GM) const;
	bool HasLineOfSight(ANightShiftCharacter* Player, const FVector& Target) const;

	float StrafeSign = 1.f;
	float StrafeFlipIn = 1.8f;
	float AimNoisePhase = 0.f;
	bool bFiring = false;
	TWeakObjectPtr<AAlienBot> CurrentTarget;
	float RetargetIn = 0.f;
};
