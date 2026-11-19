#include "UI/HLWAccessMessageWidget.h"

#include "Components/TextBlock.h"

void UHLWAccessMessageWidget::BuildDefaultLayout()
{
	UVerticalBox* Box = MakeDefaultRoot(FVector2D(0.5f, 0.72f));
	TitleText = MakeText(Box, TEXT("TitleText"), 22, FLinearColor::White);
	MessageText = MakeText(Box, TEXT("MessageText"), 16, FLinearColor(0.8f, 0.8f, 0.8f));
}

void UHLWAccessMessageWidget::ShowAccessMessage(EHLWAccessResult Result, const FText& Title, const FText& Message, float Duration)
{
	if (TitleText)
	{
		TitleText->SetText(Title);
		if (bTintTitle)
		{
			// Matches the reader light: green, red for card problems, amber otherwise
			const FLinearColor Color = Result == EHLWAccessResult::Granted ? GrantedColor
				: HLWAccess::IsCredentialResult(Result) ? DeniedColor
				: LockedColor;
			TitleText->SetColorAndOpacity(FSlateColor(Color));
		}
	}
	if (MessageText)
	{
		MessageText->SetText(Message);
	}

	OnAccessMessage(Result);
	ShowFor(Duration);
}
