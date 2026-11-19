#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Access/HLWAccessTypes.h"
#include "HLWAccessFeedbackComponent.generated.h"

class APlayerController;
class UHLWAccessMessageSet;
class UHLWAccessMessageWidget;
class UHLWInteractionPromptWidget;
class UHLWKeycardDefinition;
class UHLWKeycardNotificationWidget;

// Shows keycard pickups, access results and the interaction prompt for the local player
UCLASS(ClassGroup = (HLW), meta = (BlueprintSpawnableComponent))
class THE_HOLLOW_API UHLWAccessFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHLWAccessFeedbackComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Swap these for the WBP versions; the C++ classes show a plain default layout
	UPROPERTY(EditAnywhere, Category = "HLW|UI|Widgets")
	TSubclassOf<UHLWAccessMessageWidget> GrantedWidgetClass;

	UPROPERTY(EditAnywhere, Category = "HLW|UI|Widgets")
	TSubclassOf<UHLWAccessMessageWidget> DeniedWidgetClass;

	UPROPERTY(EditAnywhere, Category = "HLW|UI|Widgets")
	TSubclassOf<UHLWAccessMessageWidget> OfflineWidgetClass;

	UPROPERTY(EditAnywhere, Category = "HLW|UI|Widgets")
	TSubclassOf<UHLWKeycardNotificationWidget> KeycardNotificationClass;

	UPROPERTY(EditAnywhere, Category = "HLW|UI|Widgets")
	TSubclassOf<UHLWInteractionPromptWidget> PromptWidgetClass;

	// Optional wording overrides
	UPROPERTY(EditAnywhere, Category = "HLW|UI")
	TObjectPtr<UHLWAccessMessageSet> MessageSet;

	UPROPERTY(EditAnywhere, Category = "HLW|UI", meta = (ClampMin = "0.1"))
	float AccessMessageDuration = 2.5f;

	UPROPERTY(EditAnywhere, Category = "HLW|UI", meta = (ClampMin = "0.1"))
	float KeycardNotificationDuration = 3.5f;

	UPROPERTY(EditAnywhere, Category = "HLW|UI")
	int32 WidgetZOrder = 10;

	// Until the Item Interaction System owns focus tracing
	UPROPERTY(EditAnywhere, Category = "HLW|UI|Prompt")
	bool bShowInteractionPrompt = true;

	UPROPERTY(EditAnywhere, Category = "HLW|UI|Prompt", meta = (ClampMin = "0", EditCondition = "bShowInteractionPrompt"))
	float PromptRange = 250.f;

	UPROPERTY(EditAnywhere, Category = "HLW|UI|Prompt", meta = (ClampMin = "0.02", EditCondition = "bShowInteractionPrompt"))
	float PromptInterval = 0.1f;

	// Shown before the prompt, empty hides it
	UPROPERTY(EditAnywhere, Category = "HLW|UI|Prompt", meta = (EditCondition = "bShowInteractionPrompt"))
	FText PromptKeyLabel;

private:
	UFUNCTION()
	void HandleKeycardAdded(UHLWKeycardDefinition* Keycard);

	UFUNCTION()
	void HandleAccessResolved(FName DeviceID, EHLWAccessResult Result);

	void UpdatePrompt();
	APlayerController* GetLocalPlayerController() const;

	template <typename WidgetT>
	WidgetT* GetOrCreateWidget(TSubclassOf<WidgetT> WidgetClass, TObjectPtr<WidgetT>& Cache);

	UPROPERTY(Transient)
	TObjectPtr<UHLWAccessMessageWidget> GrantedWidget;

	UPROPERTY(Transient)
	TObjectPtr<UHLWAccessMessageWidget> DeniedWidget;

	UPROPERTY(Transient)
	TObjectPtr<UHLWAccessMessageWidget> OfflineWidget;

	UPROPERTY(Transient)
	TObjectPtr<UHLWKeycardNotificationWidget> NotificationWidget;

	UPROPERTY(Transient)
	TObjectPtr<UHLWInteractionPromptWidget> PromptWidget;

	FTimerHandle PromptTimer;
	FText CurrentPrompt;
};
