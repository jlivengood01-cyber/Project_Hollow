#include "UI/HLWKeycardNotificationWidget.h"

#include "Access/HLWKeycardDefinition.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "HLWKeycardNotification"

void UHLWKeycardNotificationWidget::BuildDefaultLayout()
{
	UVerticalBox* Box = MakeDefaultRoot(FVector2D(0.5f, 0.18f));
	HeaderText = MakeText(Box, TEXT("HeaderText"), 14, FLinearColor(0.7f, 0.7f, 0.7f));
	CardNameText = MakeText(Box, TEXT("CardNameText"), 22, FLinearColor::White);
	AccessLevelText = MakeText(Box, TEXT("AccessLevelText"), 14, FLinearColor(0.7f, 0.7f, 0.7f));
}

void UHLWKeycardNotificationWidget::ShowKeycard(UHLWKeycardDefinition* Keycard, float Duration)
{
	if (!Keycard)
	{
		return;
	}

	if (HeaderText)
	{
		HeaderText->SetText(LOCTEXT("Header", "Keycard acquired"));
	}
	if (CardNameText)
	{
		CardNameText->SetText(Keycard->DisplayName.IsEmpty() ? FText::FromName(Keycard->KeycardID) : Keycard->DisplayName);
		if (bTintCardName)
		{
			CardNameText->SetColorAndOpacity(FSlateColor(Keycard->Color));
		}
	}
	if (AccessLevelText)
	{
		AccessLevelText->SetText(FText::Format(LOCTEXT("Level", "{0} access"), UEnum::GetDisplayValueAsText(Keycard->AccessLevel)));
	}
	if (CardIcon)
	{
		if (Keycard->Icon)
		{
			CardIcon->SetBrushFromTexture(Keycard->Icon);
			CardIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			CardIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	OnKeycardShown(Keycard);
	ShowFor(Duration);
}

#undef LOCTEXT_NAMESPACE
