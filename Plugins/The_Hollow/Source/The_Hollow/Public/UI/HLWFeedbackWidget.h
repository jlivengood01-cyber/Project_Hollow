#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HLWFeedbackWidget.generated.h"

class UTextBlock;
class UVerticalBox;

// Base for short on-screen feedback. Builds a plain layout when used without a Blueprint.
UCLASS(Abstract)
class THE_HOLLOW_API UHLWFeedbackWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Duration of 0 keeps it visible until hidden
	UFUNCTION(BlueprintCallable, Category = "HLW|UI")
	void ShowFor(float Duration);

	UFUNCTION(BlueprintCallable, Category = "HLW|UI")
	void HideNow();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	// Called only when the widget has no Blueprint layout
	virtual void BuildDefaultLayout() {}

	UVerticalBox* MakeDefaultRoot(const FVector2D& ScreenAnchor);
	UTextBlock* MakeText(UVerticalBox* Box, FName Name, int32 FontSize, const FLinearColor& Color);

	// Hooks for Blueprint animations and sounds
	UFUNCTION(BlueprintImplementableEvent, Category = "HLW|UI")
	void OnShown();

	UFUNCTION(BlueprintImplementableEvent, Category = "HLW|UI")
	void OnHidden();

private:
	FTimerHandle HideTimer;
};
