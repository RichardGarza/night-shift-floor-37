#include "HUDWidget.h"
#include "ArenaGameMode.h"
#include "NightShiftCharacter.h"
#include "RifleComponent.h"
#include "NightShiftFloor37.h"

void UHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ShowStartPrompt();
}

void UHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (HitMarkerTimeRemaining > 0.f)
	{
		HitMarkerTimeRemaining -= InDeltaTime;
	}

	if (!BoundGameMode.IsValid() || !BoundCharacter.IsValid())
	{
		return;
	}

	int32 Mag = 0, Reserve = 0;
	if (BoundCharacter->Rifle)
	{
		BoundCharacter->Rifle->GetAmmo(Mag, Reserve);
	}
	const float MaxHP = 100.f; // prefer GameConfig when wired
	const float HPPct = BoundCharacter->Health / MaxHP;
	OnRefreshHUD(HPPct, Mag, Reserve, BoundGameMode->KillCount, BoundGameMode->MatchTimeSeconds);
}

void UHUDWidget::BindToMatch(AArenaGameMode* GameMode, ANightShiftCharacter* Character)
{
	BoundGameMode = GameMode;
	BoundCharacter = Character;
}

void UHUDWidget::ShowStartPrompt()
{
	OnPromptChanged(FText::FromString(TEXT("Click to play")));
}

void UHUDWidget::ShowDeathPrompt()
{
	OnPromptChanged(FText::FromString(TEXT("You died — click to restart")));
}

void UHUDWidget::ShowWin(float MatchTimeSeconds)
{
	OnPromptChanged(FText::FromString(FString::Printf(TEXT("Floor cleared — %.1fs"), MatchTimeSeconds)));
}

void UHUDWidget::ShowHitMarker(bool bHeadshot)
{
	HitMarkerTimeRemaining = 0.12f;
	(void)bHeadshot;
	// TODO: BP animates crosshair tick
}
