#include "UI/HLWFeedbackWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UHLWFeedbackWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
	SetVisibility(ESlateVisibility::Collapsed);
}

void UHLWFeedbackWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimer);
	}
	Super::NativeDestruct();
}

void UHLWFeedbackWidget::ShowFor(float Duration)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
	OnShown();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(HideTimer);
	if (Duration > 0.f)
	{
		World->GetTimerManager().SetTimer(HideTimer, this, &ThisClass::HideNow, Duration, false);
	}
}

void UHLWFeedbackWidget::HideNow()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimer);
	}
	if (GetVisibility() != ESlateVisibility::Collapsed)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		OnHidden();
	}
}

UVerticalBox* UHLWFeedbackWidget::MakeDefaultRoot(const FVector2D& ScreenAnchor)
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
	WidgetTree->RootWidget = Canvas;

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));
	UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Box);
	CanvasSlot->SetAnchors(FAnchors(ScreenAnchor.X, ScreenAnchor.Y));
	CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	CanvasSlot->SetAutoSize(true);
	return Box;
}

UTextBlock* UHLWFeedbackWidget::MakeText(UVerticalBox* Box, FName Name, int32 FontSize, const FLinearColor& Color)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);

	FSlateFontInfo Font = Text->GetFont();
	Font.Size = FontSize;
	Text->SetFont(Font);
	Text->SetColorAndOpacity(FSlateColor(Color));
	Text->SetShadowOffset(FVector2D(1.f, 1.f));
	Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
	Text->SetJustification(ETextJustify::Center);

	UVerticalBoxSlot* BoxSlot = Box->AddChildToVerticalBox(Text);
	BoxSlot->SetHorizontalAlignment(HAlign_Center);
	return Text;
}
