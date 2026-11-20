#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Electrical/HLWPowerTypes.h"
#include "HLWPowerConsumerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHLWOnConsumerPowerChanged, bool, bPowered, bool, bInitial);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHLWOnConsumerPowerEvent, bool, bInitial);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHLWOnConsumerCircuitState, EHLWPowerState, State, bool, bInitial);

// Add to anything that needs electricity. No CircuitID = always powered.
UCLASS(ClassGroup = (HLW), meta = (BlueprintSpawnableComponent))
class THE_HOLLOW_API UHLWPowerConsumerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHLWPowerConsumerComponent();

	UFUNCTION(BlueprintPure, Category = "HLW|Power")
	bool IsPowered() const { return bPowered; }

	// True when the circuit is set, so the power system drives this consumer
	UFUNCTION(BlueprintPure, Category = "HLW|Power")
	bool UsesCircuit() const { return !ConsumerData.CircuitID.IsNone(); }

	UFUNCTION(BlueprintPure, Category = "HLW|Power")
	EHLWPowerState GetCircuitState() const { return CircuitState; }

	UFUNCTION(BlueprintPure, Category = "HLW|Power")
	FName GetCircuitID() const { return ConsumerData.CircuitID; }

	UFUNCTION(BlueprintCallable, Category = "HLW|Power")
	void SetCircuitID(FName NewCircuitID);

	const FHLWPowerConsumerData& GetConsumerData() const { return ConsumerData; }

	// Called by the power subsystem
	void ReceivePowerUpdate(EHLWPowerState NewCircuitState, bool bInitial);

	// bInitial is true for the starting state, so owners can skip sounds and effects
	UPROPERTY(BlueprintAssignable, Category = "HLW|Power")
	FHLWOnConsumerPowerChanged OnPowerChanged;

	UPROPERTY(BlueprintAssignable, Category = "HLW|Power")
	FHLWOnConsumerPowerEvent OnPowerRestored;

	UPROPERTY(BlueprintAssignable, Category = "HLW|Power")
	FHLWOnConsumerPowerEvent OnPowerLost;

	// Every circuit state change, e.g. Powered to Backup while still running
	UPROPERTY(BlueprintAssignable, Category = "HLW|Power")
	FHLWOnConsumerCircuitState OnCircuitStateChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Power")
	FHLWPowerConsumerData ConsumerData;

private:
	void Register();
	void Unregister();
	void SetPowered(bool bNewPowered, bool bInitial);

	UPROPERTY(VisibleInstanceOnly, Category = "HLW|Power")
	bool bPowered = true;

	UPROPERTY(VisibleInstanceOnly, Category = "HLW|Power")
	EHLWPowerState CircuitState = EHLWPowerState::Powered;

	bool bHasReceivedState = false;
};
