// Night Shift — Floor 37 | Crosshair, HP, ammo, prompts
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDWidget.generated.h"

class AArenaGameMode;
class ANightShiftCharacter;

/**
 * UMG overlay: crosshair, HP bar, ammo "30 / 90", kill count, timer,
 * "Click to play", "You died — click to restart", win at 25 kills with time.
 */
UCLASS()
class NIGHTSHIFTFLOOR37_API UHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void BindToMatch(AArenaGameMode* GameMode, ANightShiftCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowStartPrompt();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowDeathPrompt();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowWin(float MatchTimeSeconds);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowHitMarker(bool bHeadshot);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnRefreshHUD(float HealthPercent, int32 Mag, int32 Reserve, int32 Kills, float TimeSeconds);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnPromptChanged(const FText& Prompt);

protected:
	UPROPERTY()
	TWeakObjectPtr<AArenaGameMode> BoundGameMode;

	UPROPERTY()
	TWeakObjectPtr<ANightShiftCharacter> BoundCharacter;

	float HitMarkerTimeRemaining = 0.f;
};
