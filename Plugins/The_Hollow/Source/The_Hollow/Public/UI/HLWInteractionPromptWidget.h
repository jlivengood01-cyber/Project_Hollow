#pragma once

#include "CoreMinimal.h"
#include "UI/HLWFeedbackWidget.h"
#include "HLWInteractionPromptWidget.generated.h"

class UTextBlock;

// Base for WBP_InteractionPrompt
UCLASS()
class THE_HOLLOW_API UHLWInteractionPromptWidget : public UHLWFeedbackWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HLW|UI")
	void SetPrompt(const FText& Prompt);

	UFUNCTION(BlueprintCallable, Category = "HLW|UI")
	void ClearPrompt();

protected:
	virtual void BuildDefaultLayout() override;

	UPROPERTY(BlueprintReadOnly, Category = "HLW|UI", meta = (BindWidget))
	TObjectPtr<UTextBlock> PromptText;
};
