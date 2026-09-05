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
#include "Components/Border.h"
#include "Components/Slider.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Kismet/KismetSystemLibrary.h"

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

	// Full-screen red overlay; alpha driven by time since last damage. Added first so it sits under everything.
	DamageVignette = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DamageVignette"));
	DamageVignette->SetBrushColor(FLinearColor(0.85f, 0.06f, 0.03f, 0.f));
	DamageVignette->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(DamageVignette))
	{
		S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		S->SetOffsets(FMargin(0.f));
	}

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

	BuildPauseMenu(Canvas);
}

void UHUDWidget::BuildPauseMenu(UCanvasPanel* Canvas)
{
	PausePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PausePanel"));
	PausePanel->SetBrushColor(FLinearColor(0.02f, 0.05f, 0.04f, 0.88f));
	PausePanel->SetPadding(FMargin(32.f, 24.f));
	PausePanel->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(PausePanel))
	{
		S->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		S->SetAlignment(FVector2D(0.5f, 0.5f));
		S->SetPosition(FVector2D(0.f, 0.f));
		S->SetAutoSize(true);
		S->SetZOrder(10);
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PauseBox"));
	PausePanel->SetContent(Box);

	auto AddRow = [Box](UWidget* W, float TopPad, EHorizontalAlignment HAlign)
	{
		if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(W))
		{
			S->SetPadding(FMargin(0.f, TopPad, 0.f, 0.f));
			S->SetHorizontalAlignment(HAlign);
		}
	};

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PauseTitle"));
	{
		FSlateFontInfo F = Title->GetFont();
		F.Size = 30;
		Title->SetFont(F);
	}
	Title->SetColorAndOpacity(FSlateColor(HUDPrivate::Green));
	Title->SetText(FText::FromString(TEXT("Paused")));
	AddRow(Title, 0.f, HAlign_Center);

	SensitivityLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SensitivityLabel"));
	{
		FSlateFontInfo F = SensitivityLabel->GetFont();
		F.Size = 14;
		SensitivityLabel->SetFont(F);
	}
	SensitivityLabel->SetColorAndOpacity(FSlateColor(HUDPrivate::Dim));
	RefreshSensitivityLabel(0.35f);
	AddRow(SensitivityLabel, 18.f, HAlign_Left);

	USizeBox* SliderBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SliderBox"));
	SliderBox->SetWidthOverride(340.f);
	SensitivitySlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("SensitivitySlider"));
	SensitivitySlider->SetMinValue(0.05f);
	SensitivitySlider->SetMaxValue(1.5f);
	SensitivitySlider->SetStepSize(0.05f);
	SensitivitySlider->SetValue(0.35f);
	SensitivitySlider->SetSliderBarColor(FLinearColor(0.2f, 0.3f, 0.25f));
	SensitivitySlider->SetSliderHandleColor(HUDPrivate::Green);
	SensitivitySlider->OnValueChanged.AddDynamic(this, &UHUDWidget::HandleSensitivityChanged);
	SliderBox->SetContent(SensitivitySlider);
	AddRow(SliderBox, 6.f, HAlign_Fill);

	UButton* Resume = MakeButton(Box, TEXT("ResumeButton"), TEXT("Resume  (Esc)"));
	Resume->OnClicked.AddDynamic(this, &UHUDWidget::HandleResumeClicked);
	UButton* Quit = MakeButton(Box, TEXT("QuitButton"), TEXT("Quit to desktop"));
	Quit->OnClicked.AddDynamic(this, &UHUDWidget::HandleQuitClicked);
}

UButton* UHUDWidget::MakeButton(UVerticalBox* Box, const FName& Name, const FString& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	Button->SetBackgroundColor(FLinearColor(0.55f, 0.85f, 0.6f, 1.f));
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Name.ToString() + TEXT("_Label")));
	{
		FSlateFontInfo F = Text->GetFont();
		F.Size = 15;
		Text->SetFont(F);
	}
	Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.03f, 0.08f, 0.05f)));
	Text->SetText(FText::FromString(Label));
	Button->AddChild(Text);
	if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Button))
	{
		S->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
		S->SetHorizontalAlignment(HAlign_Fill);
	}
	return Button;
}

void UHUDWidget::RefreshSensitivityLabel(float Value)
{
	if (SensitivityLabel)
	{
		SensitivityLabel->SetText(FText::FromString(FString::Printf(TEXT("Mouse sensitivity   %.2f"), Value)));
	}
}

void UHUDWidget::HandleSensitivityChanged(float Value)
{
	if (BoundCharacter.IsValid())
	{
		BoundCharacter->SetMouseSensitivity(Value);
		Value = BoundCharacter->MouseSensitivity;
	}
	RefreshSensitivityLabel(Value);
}

void UHUDWidget::HandleResumeClicked()
{
	if (BoundGameMode.IsValid())
	{
		BoundGameMode->PauseMatch(false);
	}
}

void UHUDWidget::HandleQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
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
	const bool bPaused = BoundGameMode->IsMatchPaused();
	if (PausePanel)
	{
		if (bPaused && !bPausePanelShown)
		{
			if (SensitivitySlider)
			{
				SensitivitySlider->SetValue(BoundCharacter->MouseSensitivity);
			}
			RefreshSensitivityLabel(BoundCharacter->MouseSensitivity);
		}
		bPausePanelShown = bPaused;
		PausePanel->SetVisibility(bPaused ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (DamageVignette)
	{
		const float FadeSec = (Cfg ? Cfg->DamageVignetteDurationMs : 500.f) * 0.001f;
		float Alpha = FadeSec > 0.f ? FMath::Clamp(1.f - BoundCharacter->TimeSinceLastDamage / FadeSec, 0.f, 1.f) * 0.45f : 0.f;
		if (!BoundCharacter->IsAlive())
		{
			Alpha = 0.4f;
		}
		DamageVignette->SetBrushColor(FLinearColor(0.85f, 0.06f, 0.03f, Alpha));
	}
	if (PromptText)
	{
		PromptText->SetText(CurrentPrompt);
		const bool bShowPrompt = !bPaused && !CurrentPrompt.IsEmpty();
		PromptText->SetVisibility(bShowPrompt ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (PromptHintText)
		{
			PromptHintText->SetVisibility(bShowPrompt ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (CrosshairText)
		{
			CrosshairText->SetVisibility((bShowPrompt || bPaused) ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
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
	const UGameConfig* Cfg = BoundCharacter.IsValid() ? BoundCharacter->GameConfig.Get() : nullptr;
	HitMarkerTimeRemaining = (Cfg ? Cfg->HitMarkerDurationMs : 120.f) * 0.001f;
	bLastHitWasHeadshot = bHeadshot;
}
