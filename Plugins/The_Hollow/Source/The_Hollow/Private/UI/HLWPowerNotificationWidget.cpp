#include "UI/HLWPowerNotificationWidget.h"

#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "HLWPowerNotification"

void UHLWPowerNotificationWidget::BuildDefaultLayout()
{
	UVerticalBox* Box = MakeDefaultRoot(FVector2D(0.5f, 0.28f));
	TitleText = MakeText(Box, TEXT("TitleText"), 22, FLinearColor::White);
	AreaText = MakeText(Box, TEXT("AreaText"), 14, FLinearColor(0.75f, 0.75f, 0.75f));
}

void UHLWPowerNotificationWidget::ShowPowerNotice(EHLWPowerNoticeType Type, const FText& InAreaText, float Duration)
{
	if (TitleText)
	{
		TitleText->SetText(GetNoticeTitle(Type));
		TitleText->SetColorAndOpacity(FSlateColor(GetNoticeColor(Type)));
	}
	if (AreaText)
	{
		AreaText->SetText(InAreaText);
		AreaText->SetVisibility(InAreaText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	OnPowerNoticeShown(Type);
	ShowFor(Duration);
}

FText UHLWPowerNotificationWidget::GetNoticeTitle(EHLWPowerNoticeType Type)
{
	switch (Type)
	{
	case EHLWPowerNoticeType::Restored: return LOCTEXT("Restored", "Power restored");
	case EHLWPowerNoticeType::Backup:   return LOCTEXT("Backup", "Emergency power");
	case EHLWPowerNoticeType::Failure:  return LOCTEXT("Failure", "Power failure");
	case EHLWPowerNoticeType::Fault:    return LOCTEXT("Fault", "Electrical fault");
	default:                            return FText::GetEmpty();
	}
}

FLinearColor UHLWPowerNotificationWidget::GetNoticeColor(EHLWPowerNoticeType Type)
{
	switch (Type)
	{
	case EHLWPowerNoticeType::Restored: return FLinearColor(0.2f, 1.f, 0.35f);
	case EHLWPowerNoticeType::Backup:   return FLinearColor(1.f, 0.6f, 0.1f);
	case EHLWPowerNoticeType::Failure:  return FLinearColor(1.f, 0.2f, 0.15f);
	case EHLWPowerNoticeType::Fault:    return FLinearColor(1.f, 0.45f, 0.f);
	default:                            return FLinearColor::White;
	}
}

#undef LOCTEXT_NAMESPACE
