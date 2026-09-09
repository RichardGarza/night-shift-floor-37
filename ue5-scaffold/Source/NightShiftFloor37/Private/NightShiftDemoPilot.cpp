#include "NightShiftDemoPilot.h"
#include "ArenaGameMode.h"
#include "NightShiftCharacter.h"
#include "AlienBot.h"
#include "RifleComponent.h"
#include "GameConfig.h"
#include "OfficeArena.h"
#include "NightShiftFloor37.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"

ANightShiftDemoPilot::ANightShiftDemoPilot()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics; // before the character consumes movement input
}

bool ANightShiftDemoPilot::HasLineOfSight(ANightShiftCharacter* Player, const FVector& Target) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DemoLOS), false, Player);
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Player->GetAimOrigin(), Target, ECC_Visibility, Params);
	return !bBlocked || Cast<AAlienBot>(Hit.GetActor()) != nullptr;
}

AAlienBot* ANightShiftDemoPilot::PickTarget(ANightShiftCharacter* Player, AArenaGameMode* GM) const
{
	AAlienBot* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	const FVector Eye = Player->GetAimOrigin();
	const FVector Fwd = Player->GetAimDirection();
	for (TActorIterator<AAlienBot> It(GetWorld()); It; ++It)
	{
		AAlienBot* Bot = *It;
		if (!Bot || !Bot->bIsAlive)
		{
			continue;
		}
		const FVector To = Bot->GetActorLocation() - Eye;
		const float Dist = To.Size();
		// Prefer close and roughly ahead; visible targets only.
		const float Facing = FVector::DotProduct(Fwd, To.GetSafeNormal()); // 1 = dead ahead
		const float Score = Dist * (1.6f - Facing);
		if (Score < BestScore && HasLineOfSight(Player, Bot->GetActorLocation() + FVector(0.f, 0.f, 40.f)))
		{
			BestScore = Score;
			Best = Bot;
		}
	}
	return Best;
}

void ANightShiftDemoPilot::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AArenaGameMode* GM = Cast<AArenaGameMode>(UGameplayStatics::GetGameMode(this));
	ANightShiftCharacter* Player = GM ? Cast<ANightShiftCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)) : nullptr;
	if (!GM || !Player)
	{
		return;
	}
	Elapsed += DeltaSeconds;
	if (GM->MatchState != EArenaMatchState::InProgress || GM->IsMatchPaused() || !Player->IsAlive())
	{
		if (bFiring) { Player->StopFire(); bFiring = false; }
		return;
	}
	APlayerController* PC = Cast<APlayerController>(Player->GetController());
	if (!PC)
	{
		return;
	}
	const UGameConfig* Cfg = GM->GameConfig;

	// ---- target ----
	RetargetIn -= DeltaSeconds;
	if (!CurrentTarget.IsValid() || !CurrentTarget->bIsAlive || RetargetIn <= 0.f)
	{
		CurrentTarget = PickTarget(Player, GM);
		RetargetIn = 0.7f;
	}
	AAlienBot* Target = CurrentTarget.Get();

	// ---- aim ----
	const FVector Eye = Player->GetAimOrigin();
	FRotator Want = PC->GetControlRotation();
	float AimErrorDeg = 180.f;
	if (Target)
	{
		const float HalfH = Target->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		const FVector AimPoint = Target->GetActorLocation() + FVector(0.f, 0.f, HalfH * 0.55f); // upper chest / head
		Want = (AimPoint - Eye).Rotation();
		// A little hand-wobble so it does not look like a turret.
		AimNoisePhase += DeltaSeconds;
		Want.Yaw += FMath::Sin(AimNoisePhase * 5.3f) * 0.6f;
		Want.Pitch += FMath::Sin(AimNoisePhase * 3.1f + 1.f) * 0.4f;
		const FRotator Cur = PC->GetControlRotation();
		AimErrorDeg = FMath::Abs(FMath::FindDeltaAngleDegrees(Cur.Yaw, Want.Yaw)) + FMath::Abs(FMath::FindDeltaAngleDegrees(Cur.Pitch, Want.Pitch));
		PC->SetControlRotation(FMath::RInterpTo(Cur, Want, DeltaSeconds, 9.f));
	}
	else
	{
		// Nobody visible: sweep slowly toward the atrium.
		const FVector Centre = GM->GetOfficeArena() ? GM->GetOfficeArena()->GetActorLocation() : FVector::ZeroVector;
		FRotator ToCentre = (Centre + FVector(0.f, 0.f, 150.f) - Eye).Rotation();
		ToCentre.Yaw += FMath::Sin(Elapsed * 0.8f) * 35.f;
		ToCentre.Pitch = -6.f;
		PC->SetControlRotation(FMath::RInterpTo(PC->GetControlRotation(), ToCentre, DeltaSeconds, 2.f));
	}

	// ---- fire ----
	int32 Mag = 0, Reserve = 0;
	if (Player->Rifle) { Player->Rifle->GetAmmo(Mag, Reserve); }
	const float DistM = Target ? FVector::Dist(Target->GetActorLocation(), Player->GetActorLocation()) * 0.01f : 999.f;
	const bool bWantFire = Target && AimErrorDeg < 3.5f && DistM < 32.f && Mag > 0;
	if (bWantFire != bFiring)
	{
		bFiring = bWantFire;
		if (bFiring) { Player->StartFire(); } else { Player->StopFire(); }
	}
	if (!Target && Mag < (Cfg ? Cfg->MagSize : 30) && Reserve > 0)
	{
		Player->RequestReload(); // top up between fights
	}

	// ---- move ----
	FVector Move = FVector::ZeroVector;
	const FVector Loc = Player->GetActorLocation();
	const FVector Centre = GM->GetOfficeArena() ? GM->GetOfficeArena()->GetActorLocation() : FVector::ZeroVector;
	StrafeFlipIn -= DeltaSeconds;
	if (StrafeFlipIn <= 0.f)
	{
		StrafeSign *= -1.f;
		StrafeFlipIn = FMath::FRandRange(1.4f, 2.6f);
	}
	if (Target)
	{
		const FVector ToTgt = (Target->GetActorLocation() - Loc).GetSafeNormal2D();
		const FVector Right = FVector::CrossProduct(FVector::UpVector, ToTgt);
		Move += Right * StrafeSign * 0.9f;                       // strafe around the target
		if (DistM < 7.f)       { Move -= ToTgt * 1.0f; }          // too close: back off
		else if (DistM > 15.f) { Move += ToTgt * 0.8f; }          // too far: close in
	}
	else
	{
		// Drift toward the atrium so the camera has something to look at.
		const FVector ToCentre = (Centre - Loc);
		if (ToCentre.Size2D() > 900.f) { Move += ToCentre.GetSafeNormal2D() * 0.7f; }
		Move += FVector(FMath::Sin(Elapsed * 0.9f), FMath::Cos(Elapsed * 0.7f), 0.f) * 0.4f;
	}
	// Stay off the walls / out of the corners.
	const FVector FromCentre = Loc - Centre;
	const float Half = Cfg ? Cfg->ArenaSizeMeters * 50.f : 2500.f;
	if (FMath::Abs(FromCentre.X) > Half - 500.f) { Move.X -= FMath::Sign(FromCentre.X) * 1.2f; }
	if (FMath::Abs(FromCentre.Y) > Half - 500.f) { Move.Y -= FMath::Sign(FromCentre.Y) * 1.2f; }
	if (!Move.IsNearlyZero())
	{
		Move = Move.GetClampedToMaxSize(1.f);
		Player->AddMovementInput(Move, 1.f);
	}
	// Sprint only when nobody is in sight (firing drops sprint anyway).
	if (!Target && !Player->bWantsSprint) { Player->StartSprint(); }
	else if (Target && Player->bWantsSprint) { Player->StopSprint(); }
}
