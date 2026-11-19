#include "UI/HLWInteractionPromptWidget.h"

#include "Components/TextBlock.h"

void UHLWInteractionPromptWidget::BuildDefaultLayout()
{
	UVerticalBox* Box = MakeDefaultRoot(FVector2D(0.5f, 0.58f));
	PromptText = MakeText(Box, TEXT("PromptText"), 16, FLinearColor(0.85f, 0.85f, 0.85f));
}

void UHLWInteractionPromptWidget::SetPrompt(const FText& Prompt)
{
	if (PromptText)
	{
		PromptText->SetText(Prompt);
	}
	ShowFor(0.f);
}

void UHLWInteractionPromptWidget::ClearPrompt()
{
	HideNow();
}
