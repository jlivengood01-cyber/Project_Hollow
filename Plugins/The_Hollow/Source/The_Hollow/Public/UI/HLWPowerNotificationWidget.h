#pragma once

#include "CoreMinimal.h"
#include "UI/HLWFeedbackWidget.h"
#include "Electrical/HLWPowerTypes.h"
#include "HLWPowerNotificationWidget.generated.h"

class UTextBlock;

// Base for WBP_PowerNotification: Power restored / Power failure / Electrical fault, with the area
UCLASS()
class THE_HOLLOW_API UHLWPowerNotificationWidget : public UHLWFeedbackWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HLW|UI")
	void ShowPowerNotice(EHLWPowerNoticeType Type, const FText& AreaText, float Duration);

	UFUNCTION(BlueprintPure, Category = "HLW|UI")
	static FText GetNoticeTitle(EHLWPowerNoticeType Type);

	UFUNCTION(BlueprintPure, Category = "HLW|UI")
	static FLinearColor GetNoticeColor(EHLWPowerNoticeType Type);

protected:
	virtual void BuildDefaultLayout() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "HLW|UI")
	void OnPowerNoticeShown(EHLWPowerNoticeType Type);

	UPROPERTY(BlueprintReadOnly, Category = "HLW|UI", meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(BlueprintReadOnly, Category = "HLW|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AreaText;
};
