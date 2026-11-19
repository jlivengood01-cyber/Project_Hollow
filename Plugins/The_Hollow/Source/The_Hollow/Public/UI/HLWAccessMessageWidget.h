#pragma once

#include "CoreMinimal.h"
#include "UI/HLWFeedbackWidget.h"
#include "Access/HLWAccessTypes.h"
#include "HLWAccessMessageWidget.generated.h"

class UTextBlock;

// Base for WBP_AccessGranted, WBP_AccessDenied and WBP_KeycardReaderStatus
UCLASS()
class THE_HOLLOW_API UHLWAccessMessageWidget : public UHLWFeedbackWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HLW|UI")
	void ShowAccessMessage(EHLWAccessResult Result, const FText& Title, const FText& Message, float Duration);

protected:
	virtual void BuildDefaultLayout() override;

	// Per-result styling in Blueprints
	UFUNCTION(BlueprintImplementableEvent, Category = "HLW|UI")
	void OnAccessMessage(EHLWAccessResult Result);

	UPROPERTY(BlueprintReadOnly, Category = "HLW|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(BlueprintReadOnly, Category = "HLW|UI", meta = (BindWidget))
	TObjectPtr<UTextBlock> MessageText;

	// Turn off to keep the title color set in the Blueprint
	UPROPERTY(EditAnywhere, Category = "HLW|UI")
	bool bTintTitle = true;

	UPROPERTY(EditAnywhere, Category = "HLW|UI", meta = (EditCondition = "bTintTitle"))
	FLinearColor GrantedColor = FLinearColor(0.45f, 0.85f, 0.5f);

	UPROPERTY(EditAnywhere, Category = "HLW|UI", meta = (EditCondition = "bTintTitle"))
	FLinearColor DeniedColor = FLinearColor(0.8f, 0.2f, 0.15f);

	// Lockdown, disabled access and offline readers
	UPROPERTY(EditAnywhere, Category = "HLW|UI", meta = (EditCondition = "bTintTitle"))
	FLinearColor LockedColor = FLinearColor(0.85f, 0.6f, 0.2f);
};
