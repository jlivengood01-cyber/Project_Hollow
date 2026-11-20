#pragma once

#include "CoreMinimal.h"
#include "Electrical/HLWCircuitSwitch.h"
#include "HLWFusebox.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHLWOnFuseInstalled, FName, FuseType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHLWOnFuseboxInspected);

// Breaker that needs a fuse before it can switch on.
// Flow: inspect, install a fuse from the player's item holder, switch on.
UCLASS()
class THE_HOLLOW_API AHLWFusebox : public AHLWCircuitSwitch
{
	GENERATED_BODY()

public:
	// Takes the first accepted fuse the actor holds
	UFUNCTION(BlueprintCallable, Category = "HLW|Fusebox")
	bool InstallFuse(AActor* FromActor);

	UFUNCTION(BlueprintPure, Category = "HLW|Fusebox")
	bool HasFuse() const { return !InstalledFuseType.IsNone(); }

	UFUNCTION(BlueprintPure, Category = "HLW|Fusebox")
	bool AcceptsFuse(FName FuseType) const { return AcceptedFuseTypes.Contains(FuseType); }

	UFUNCTION(BlueprintPure, Category = "HLW|Fusebox")
	bool WasInspected() const { return bInspected; }

	UFUNCTION(BlueprintPure, Category = "HLW|Fusebox")
	bool IsFuseboxFaulted() const { return bFaultedNow; }

	UFUNCTION(BlueprintPure, Category = "HLW|Fusebox")
	EHLWFuseboxState GetFuseboxState() const;

	// Faults the circuit and switches the box off
	UFUNCTION(BlueprintCallable, Category = "HLW|Fusebox")
	void SetFaulted(bool bNewFaulted);

	// Accepted fuse held by this actor, or None
	UFUNCTION(BlueprintPure, Category = "HLW|Fusebox")
	FName FindHeldFuse(const AActor* Holder) const;

	virtual void Interact_Implementation(AActor* InteractingActor) override;
	virtual FText GetInteractionText_Implementation() override;
	virtual bool CanInteract_Implementation() override;

	UPROPERTY(BlueprintAssignable, Category = "HLW|Fusebox")
	FHLWOnFuseInstalled OnFuseInstalled;

	UPROPERTY(BlueprintAssignable, Category = "HLW|Fusebox")
	FHLWOnFuseboxInspected OnInspected;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool CanSwitchOn() const override { return HasFuse() && !bFaultedNow; }
	virtual void ReadSaveData(const FHLWSwitchSaveData* Saved) override;
	virtual void WriteSaveData(FHLWSwitchSaveData& Out) const override;
	virtual void LoadStartingState() override;

	// Fuse types (item IDs) this box takes
	UPROPERTY(EditAnywhere, Category = "HLW|Fusebox")
	TArray<FName> AcceptedFuseTypes = { TEXT("Fuse.Standard") };

	UPROPERTY(EditAnywhere, Category = "HLW|Fusebox")
	bool bStartsWithFuse = false;

	// Starting state, see IsFuseboxFaulted for the live one
	UPROPERTY(EditAnywhere, Category = "HLW|Fusebox", meta = (DisplayName = "Starts Faulted"))
	bool bFaulted = false;

	UPROPERTY(EditAnywhere, Category = "HLW|Fusebox|Feedback")
	TObjectPtr<USoundBase> InstallSound;

	UPROPERTY(VisibleInstanceOnly, Category = "HLW|Fusebox")
	FName InstalledFuseType;

private:
	void ApplyFault() const;
	bool PlayerHoldsFuse() const;

	UPROPERTY(VisibleInstanceOnly, Category = "HLW|Fusebox")
	bool bFaultedNow = false;

	bool bInspected = false;
};
