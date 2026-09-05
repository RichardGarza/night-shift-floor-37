#include "HUDWidget.h"
#include "ArenaGameMode.h"
#include "NightShiftCharacter.h"
#include "RifleComponent.h"
#include "GameConfig.h"
#include "NightShiftFloor37.h"
#include "InputCoreTypes.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

namespace HUDPrivate
{
	const FLinearColor Ink(0.85f, 0.95f, 0.85f, 0.95f);
	const FLinearColor Dim(0.85f, 0.95f, 0.85f, 0.6f);
	const FLinearColor Green(0.45f, 0.95f, 0.5f, 1.f);
	const FLinearColor Amber(1.f, 0.75f, 0.35f, 1.f);
	const FLinearColor Red(1.f, 0.35f, 0.3f, 1.f);

	FString FormatTime(float Seconds)
	{
		const int32 S = FMath::Max(0, FMath::FloorToInt(Seconds));
		return FString::Printf(TEXT("%d:%02d"), S / 60, S % 60);
	}
}

TSharedRef<SWidget> UHUDWidget::RebuildWidget()
{
	BuildNativeTree();
	return Super::RebuildWidget();
}

void UHUDWidget::BuildNativeTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return; // Blueprint designer tree present — leave it alone
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("NativeRoot"));
	WidgetTree->RootWidget = Canvas;

	// Bottom-left: HP label + bar
	HPText = MakeText(Canvas, TEXT("HPText"), FVector2D(0.f, 1.f), FVector2D(0.f, 1.f), FVector2D(28.f, -58.f), 16, HUDPrivate::Dim);
	HPBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HPBar"));
	if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(HPBar))
	{
		S->SetAnchors(FAnchors(0.f, 1.f, 0.f, 1.f));
		S->SetAlignment(FVector2D(0.f, 1.f));
		S->SetPosition(FVector2D(28.f, -28.f));
		S->SetSize(FVector2D(260.f, 14.f));
	}
	HPBar->SetPercent(1.f);
	HPBar->SetFillColorAndOpacity(HUDPrivate::Green);

	// Bottom-right: ammo
	AmmoText = MakeText(Canvas, TEXT("AmmoText"), FVector2D(1.f, 1.f), FVector2D(1.f, 1.f), FVector2D(-28.f, -28.f), 30, HUDPrivate::Ink);

	// Top-right: kills + timer
	KillsText = MakeText(Canvas, TEXT("KillsText"), FVector2D(1.f, 0.f), FVector2D(1.f, 0.f), FVector2D(-24.f, 20.f), 16, HUDPrivate::Dim);
	TimerText = MakeText(Canvas, TEXT("TimerText"), FVector2D(1.f, 0.f), FVector2D(1.f, 0.f), FVector2D(-24.f, 42.f), 16, HUDPrivate::Dim);

	// Centre: crosshair + prompt
	CrosshairText = MakeText(Canvas, TEXT("Crosshair"), FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D(0.f, 0.f), 26, HUDPrivate::Ink);
	CrosshairText->SetText(FText::FromString(TEXT("+")));
	PromptText = MakeText(Canvas, TEXT("PromptText"), FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D(0.f, -70.f), 34, HUDPrivate::Green);
	PromptHintText = MakeText(Canvas, TEXT("PromptHint"), FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D(0.f, -28.f), 14, HUDPrivate::Dim);
	PromptHintText->SetText(FText::FromString(TEXT("WASD move · Mouse aim · LMB shoot · R reload · Shift sprint · Space jump · Q shoulder · Esc pause")));
}

UTextBlock* UHUDWidget::MakeText(UCanvasPanel* Canvas, const FName& Name, const FVector2D& Anchor, const FVector2D& Alignment,
	const FVector2D& Position, int32 FontSize, const FLinearColor& Color)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = FontSize;
	Text->SetFont(Font);
	Text->SetColorAndOpacity(FSlateColor(Color));
	Text->SetShadowOffset(FVector2D(1.f, 1.f));
	Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f));
	if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Text))
	{
		S->SetAnchors(FAnchors(Anchor.X, Anchor.Y, Anchor.X, Anchor.Y));
		S->SetAlignment(Alignment);
		S->SetPosition(Position);
		S->SetAutoSize(true);
	}
	return Text;
}

void UHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	// Visible (not SelfHitTestInvisible) so the start / restart click reaches NativeOnMouseButtonDown.
	SetVisibility(ESlateVisibility::Visible);
	ShowStartPrompt();
}

void UHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (HitMarkerTimeRemaining > 0.f)
	{
		HitMarkerTimeRemaining -= InDeltaTime;
	}
	if (CrosshairText)
	{
		CrosshairText->SetColorAndOpacity(FSlateColor(IsHitMarkerVisible()
			? (bLastHitWasHeadshot ? HUDPrivate::Red : HUDPrivate::Amber)
			: HUDPrivate::Ink));
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

	const UGameConfig* Cfg = BoundCharacter->GameConfig ? BoundCharacter->GameConfig.Get() : BoundGameMode->GameConfig.Get();
	const float MaxHP = Cfg ? Cfg->PlayerMaxHealth : 100.f;
	const int32 KillsToWin = Cfg ? Cfg->KillsToWin : 25;
	const float HPPct = (MaxHP > 0.f) ? FMath::Clamp(BoundCharacter->Health / MaxHP, 0.f, 1.f) : 0.f;

	if (HPText)
	{
		HPText->SetText(FText::FromString(FString::Printf(TEXT("HP %d"), FMath::CeilToInt(BoundCharacter->Health))));
	}
	if (HPBar)
	{
		HPBar->SetPercent(HPPct);
		HPBar->SetFillColorAndOpacity(HPPct > 0.5f ? HUDPrivate::Green : HPPct > 0.25f ? HUDPrivate::Amber : HUDPrivate::Red);
	}
	if (AmmoText)
	{
		AmmoText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Mag, Reserve)));
		AmmoText->SetColorAndOpacity(FSlateColor((Mag == 0 && Reserve == 0) ? HUDPrivate::Red : HUDPrivate::Ink));
	}
	if (KillsText)
	{
		KillsText->SetText(FText::FromString(FString::Printf(TEXT("Kills %d / %d"), BoundGameMode->KillCount, KillsToWin)));
	}
	if (TimerText)
	{
		TimerText->SetText(FText::FromString(HUDPrivate::FormatTime(BoundGameMode->MatchTimeSeconds)));
	}
	if (PromptText)
	{
		const bool bPaused = BoundGameMode->IsMatchPaused();
		PromptText->SetText(bPaused ? FText::FromString(TEXT("Paused — Esc to resume")) : CurrentPrompt);
		const bool bShowPrompt = bPaused || !CurrentPrompt.IsEmpty();
		PromptText->SetVisibility(bShowPrompt ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (PromptHintText)
		{
			PromptHintText->SetVisibility(bShowPrompt ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (CrosshairText)
		{
			CrosshairText->SetVisibility(bShowPrompt ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		}
	}

	OnRefreshHUD(HPPct, Mag, Reserve, BoundGameMode->KillCount, BoundGameMode->MatchTimeSeconds);
}

FReply UHUDWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		HandlePrimaryClick();
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UHUDWidget::BindToMatch(AArenaGameMode* GameMode, ANightShiftCharacter* Character)
{
	BoundGameMode = GameMode;
	BoundCharacter = Character;
}

void UHUDWidget::SetPrompt(const FText& Prompt)
{
	CurrentPrompt = Prompt;
	OnPromptChanged(Prompt);
}

void UHUDWidget::ShowStartPrompt()
{
	SetPrompt(FText::FromString(TEXT("Click to play")));
}

void UHUDWidget::ShowDeathPrompt()
{
	SetPrompt(FText::FromString(TEXT("You died — click to restart")));
}

void UHUDWidget::ShowWin(float MatchTimeSeconds)
{
	SetPrompt(FText::FromString(FString::Printf(TEXT("Floor cleared — %s — click to play again"), *HUDPrivate::FormatTime(MatchTimeSeconds))));
}

void UHUDWidget::ClearPrompt()
{
	SetPrompt(FText::GetEmpty());
}

void UHUDWidget::HandlePrimaryClick()
{
	if (BoundGameMode.IsValid())
	{
		BoundGameMode->RequestStartOrRestart();
	}
}

void UHUDWidget::ShowHitMarker(bool bHeadshot)
{
	HitMarkerTimeRemaining = 0.12f;
	bLastHitWasHeadshot = bHeadshot;
}
