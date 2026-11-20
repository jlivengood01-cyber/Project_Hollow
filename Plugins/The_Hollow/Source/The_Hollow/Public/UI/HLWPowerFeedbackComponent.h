#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Electrical/HLWPowerTypes.h"
#include "HLWPowerFeedbackComponent.generated.h"

class APlayerController;
class UHLWPowerNotificationWidget;
class UHLWPowerStatusWidget;

// Shows power notices and the power status panel for the local player
UCLASS(ClassGroup = (HLW), meta = (BlueprintSpawnableComponent))
class THE_HOLLOW_API UHLWPowerFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHLWPowerFeedbackComponent();

	UFUNCTION(BlueprintCallable, Category = "HLW|UI")
	void ToggleStatusPanel();

	UFUNCTION(BlueprintCallable, Category = "HLW|UI")
	void ShowNotice(EHLWPowerNoticeType Type, const FText& AreaText);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Swap these for the WBP versions; the C++ classes show a plain default layout
	UPROPERTY(EditAnywhere, Category = "HLW|UI|Widgets")
	TSubclassOf<UHLWPowerNotificationWidget> NotificationWidgetClass;

	UPROPERTY(EditAnywhere, Category = "HLW|UI|Widgets")
	TSubclassOf<UHLWPowerStatusWidget> StatusWidgetClass;

	UPROPERTY(EditAnywhere, Category = "HLW|UI", meta = (ClampMin = "0.1"))
	float NoticeDuration = 3.5f;

	UPROPERTY(EditAnywhere, Category = "HLW|UI")
	int32 WidgetZOrder = 10;

	// Circuits switched off by hand don't post a failure notice
	UPROPERTY(EditAnywhere, Category = "HLW|UI")
	bool bNoticeOnSwitchOff = false;

private:
	UFUNCTION()
	void HandleCircuitStateChanged(FName CircuitID, EHLWPowerState State, bool bInitial);

	// One notice per frame, however many circuits changed
	void FlushNotices();
	FText GetAreaName(FName CircuitID) const;
	APlayerController* GetLocalPlayerController() const;

	template <typename WidgetT>
	WidgetT* GetOrCreateWidget(TSubclassOf<WidgetT> WidgetClass, TObjectPtr<WidgetT>& Cache);

	UPROPERTY(Transient)
	TObjectPtr<UHLWPowerNotificationWidget> NotificationWidget;

	UPROPERTY(Transient)
	TObjectPtr<UHLWPowerStatusWidget> StatusWidget;

	TMap<FName, EHLWPowerState> LastStates;
	TMap<EHLWPowerNoticeType, TArray<FName>> PendingNotices;
	bool bFlushQueued = false;
};
