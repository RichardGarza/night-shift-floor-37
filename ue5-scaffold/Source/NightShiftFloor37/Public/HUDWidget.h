// Night Shift — Floor 37 | Crosshair, HP, ammo, prompts
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDWidget.generated.h"

class AArenaGameMode;
class ANightShiftCharacter;
class UCanvasPanel;
class UTextBlock;
class UProgressBar;

/**
 * UMG overlay: crosshair, HP bar, ammo "30 / 90", kill count, timer,
 * "Click to play", "You died — click to restart", win at 25 kills with time.
 * Hit-marker via ShowHitMarker / IsHitMarkerVisible.
 *
 * Builds its own widget tree in C++ (RebuildWidget), so it renders with no Blueprint.
 * A WBP subclass may still implement OnRefreshHUD / OnPromptChanged for custom art.
 */
UCLASS()
class NIGHTSHIFTFLOOR37_API UHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void BindToMatch(AArenaGameMode* GameMode, ANightShiftCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowStartPrompt();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowDeathPrompt();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowWin(float MatchTimeSeconds);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ClearPrompt();

	/** Left-click / primary → GameMode RequestStartOrRestart (start / restart). */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void HandlePrimaryClick();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowHitMarker(bool bHeadshot);

	UFUNCTION(BlueprintPure, Category = "HUD")
	bool IsHitMarkerVisible() const { return HitMarkerTimeRemaining > 0.f; }

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnRefreshHUD(float HealthPercent, int32 Mag, int32 Reserve, int32 Kills, float TimeSeconds);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnPromptChanged(const FText& Prompt);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Build the native canvas (only when no Blueprint designer tree exists). */
	void BuildNativeTree();
	void SetPrompt(const FText& Prompt);
	UTextBlock* MakeText(UCanvasPanel* Canvas, const FName& Name, const FVector2D& Anchor, const FVector2D& Alignment,
		const FVector2D& Position, int32 FontSize, const FLinearColor& Color);

	UPROPERTY()
	TWeakObjectPtr<AArenaGameMode> BoundGameMode;

	UPROPERTY()
	TWeakObjectPtr<ANightShiftCharacter> BoundCharacter;

	/** Seconds remaining for hit-marker flash; BP may poll or use IsHitMarkerVisible. */
	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	float HitMarkerTimeRemaining = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	bool bLastHitWasHeadshot = false;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	FText CurrentPrompt;

	// Native widgets (null when a Blueprint provides its own tree)
	UPROPERTY() TObjectPtr<UTextBlock> HPText;
	UPROPERTY() TObjectPtr<UProgressBar> HPBar;
	UPROPERTY() TObjectPtr<UTextBlock> AmmoText;
	UPROPERTY() TObjectPtr<UTextBlock> KillsText;
	UPROPERTY() TObjectPtr<UTextBlock> TimerText;
	UPROPERTY() TObjectPtr<UTextBlock> CrosshairText;
	UPROPERTY() TObjectPtr<UTextBlock> PromptText;
	UPROPERTY() TObjectPtr<UTextBlock> PromptHintText;
};
