#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Electrical/HLWPowerTypes.h"
#include "HLWElectricalFaultComponent.generated.h"

class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHLWOnFaultChanged, bool, bActive);

// Fault that cuts a circuit or fails a generator until repaired.
// On a generator it fails that generator. Anywhere else it faults TargetID, or the owner's circuit when empty.
UCLASS(ClassGroup = (HLW), meta = (BlueprintSpawnableComponent))
class THE_HOLLOW_API UHLWElectricalFaultComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHLWElectricalFaultComponent();

	UFUNCTION(BlueprintCallable, Category = "HLW|Fault")
	void SetFaultActive(bool bActive);

	UFUNCTION(BlueprintPure, Category = "HLW|Fault")
	bool IsFaultActive() const { return bFaultActive; }

	// Takes the repair item if one is needed. False if the repairer lacks it.
	UFUNCTION(BlueprintCallable, Category = "HLW|Fault")
	bool TryRepair(AActor* Repairer);

	UFUNCTION(BlueprintPure, Category = "HLW|Fault")
	bool CanRepair(const AActor* Repairer) const;

	// "Repair ..." or "Needs ..." for the local player
	UFUNCTION(BlueprintPure, Category = "HLW|Fault")
	FText GetRepairText() const;

	UFUNCTION(BlueprintPure, Category = "HLW|Fault")
	FName GetTargetID() const;

	UFUNCTION(BlueprintPure, Category = "HLW|Fault")
	FName GetFaultID() const;

	const FHLWElectricalFaultData& GetFaultData() const { return FaultData; }

	UPROPERTY(BlueprintAssignable, Category = "HLW|Fault")
	FHLWOnFaultChanged OnFaultChanged;

protected:
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// bActive is the starting state
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Fault")
	FHLWElectricalFaultData FaultData;

	UPROPERTY(EditAnywhere, Category = "HLW|Fault")
	bool bConsumeRepairItem = true;

	// Shown as "Needs ...". Uses the item ID when empty.
	UPROPERTY(EditAnywhere, Category = "HLW|Fault")
	FText RepairItemName;

	UPROPERTY(EditAnywhere, Category = "HLW|Fault|Feedback")
	TObjectPtr<USoundBase> FaultSound;

	UPROPERTY(EditAnywhere, Category = "HLW|Fault|Feedback")
	TObjectPtr<USoundBase> RepairSound;

	UPROPERTY(EditAnywhere, Category = "HLW|Fault|Feedback")
	bool bShowDebugResult = true;

private:
	UFUNCTION()
	void HandleStateRestored();

	bool TargetsOwnerGenerator() const;
	void ApplyCircuitFault() const;
	void PlayOneShot(USoundBase* Sound) const;
	void Report(const FString& Message) const;

	UPROPERTY(VisibleInstanceOnly, Category = "HLW|Fault")
	bool bFaultActive = false;
};
