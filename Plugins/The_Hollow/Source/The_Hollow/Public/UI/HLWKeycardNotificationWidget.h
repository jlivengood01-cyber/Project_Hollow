#pragma once

#include "CoreMinimal.h"
#include "UI/HLWFeedbackWidget.h"
#include "HLWKeycardNotificationWidget.generated.h"

class UHLWKeycardDefinition;
class UImage;
class UTextBlock;

// Base for WBP_KeycardNotification
UCLASS()
class THE_HOLLOW_API UHLWKeycardNotificationWidget : public UHLWFeedbackWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HLW|UI")
	void ShowKeycard(UHLWKeycardDefinition* Keycard, float Duration);

protected:
	virtual void BuildDefaultLayout() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "HLW|UI")
	void OnKeycardShown(UHLWKeycardDefinition* Keycard);

	UPROPERTY(BlueprintReadOnly, Category = "HLW|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HeaderText;

	UPROPERTY(BlueprintReadOnly, Category = "HLW|UI", meta = (BindWidget))
	TObjectPtr<UTextBlock> CardNameText;

	UPROPERTY(BlueprintReadOnly, Category = "HLW|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AccessLevelText;

	UPROPERTY(BlueprintReadOnly, Category = "HLW|UI", meta = (BindWidgetOptional))
	TObjectPtr<UImage> CardIcon;

	// Tints the card name with the keycard's color
	UPROPERTY(EditAnywhere, Category = "HLW|UI")
	bool bTintCardName = true;
};
