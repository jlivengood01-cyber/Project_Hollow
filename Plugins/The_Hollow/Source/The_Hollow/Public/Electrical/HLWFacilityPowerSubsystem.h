#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Electrical/HLWPowerTypes.h"
#include "HLWFacilityPowerSubsystem.generated.h"

class AHLWPowerGenerator;
class UHLWPowerConsumerComponent;

// Decides which circuits have power. Power flows Generator -> Circuit -> child Circuits -> Consumers.
// Never touches locks, lockdowns or security state.
UCLASS()
class THE_HOLLOW_API UHLWFacilityPowerSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	// Registration, called by the actors themselves in PostInitializeComponents
	void RegisterGenerator(AHLWPowerGenerator* Generator);
	void UnregisterGenerator(AHLWPowerGenerator* Generator);
	void RegisterConsumer(UHLWPowerConsumerComponent* Consumer);
	void UnregisterConsumer(UHLWPowerConsumerComponent* Consumer);

	UFUNCTION(BlueprintCallable, Category = "HLW|Power")
	void RegisterCircuit(const FHLWCircuitData& CircuitData);

	UFUNCTION(BlueprintCallable, Category = "HLW|Power")
	void UnregisterCircuit(FName CircuitID);

	UFUNCTION(BlueprintCallable, Category = "HLW|Power")
	void ActivateCircuit(FName CircuitID) { SetCircuitEnabled(CircuitID, true); }

	UFUNCTION(BlueprintCallable, Category = "HLW|Power")
	void DeactivateCircuit(FName CircuitID) { SetCircuitEnabled(CircuitID, false); }

	// Works before the circuit registers, so switches can set it in BeginPlay
	UFUNCTION(BlueprintCallable, Category = "HLW|Power")
	void SetCircuitEnabled(FName CircuitID, bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "HLW|Power")
	bool IsCircuitEnabled(FName CircuitID) const;

	// Manual fault, separate from fault components and fuseboxes
	UFUNCTION(BlueprintCallable, Category = "HLW|Power")
	void SetCircuitFaulted(FName CircuitID, bool bFaulted) { SetCircuitFaultSource(CircuitID, NAME_None, bFaulted); }

	// A circuit stays faulted until every source clears
	UFUNCTION(BlueprintCallable, Category = "HLW|Power")
	void SetCircuitFaultSource(FName CircuitID, FName SourceID, bool bFaulted);

	UFUNCTION(BlueprintPure, Category = "HLW|Power")
	bool IsCircuitFaulted(FName CircuitID) const { return FaultSources.Contains(CircuitID); }

	// Unknown circuits report Unpowered
	UFUNCTION(BlueprintPure, Category = "HLW|Power")
	EHLWPowerState GetCircuitState(FName CircuitID) const;

	// True on main power only, false on backup
	UFUNCTION(BlueprintPure, Category = "HLW|Power")
	bool IsCircuitPowered(FName CircuitID) const { return GetCircuitState(CircuitID) == EHLWPowerState::Powered; }

	// Would this circuit get power if it were switched on and fault free
	UFUNCTION(BlueprintPure, Category = "HLW|Power")
	bool HasIncomingPower(FName CircuitID) const;

	UFUNCTION(BlueprintPure, Category = "HLW|Power")
	bool GetCircuitData(FName CircuitID, FHLWCircuitData& OutData) const;

	// Display name, or the ID when no name is set
	UFUNCTION(BlueprintPure, Category = "HLW|Power")
	FText GetCircuitDisplayName(FName CircuitID) const;

	UFUNCTION(BlueprintPure, Category = "HLW|Power")
	TArray<FName> GetCircuitIDs() const;

	UFUNCTION(BlueprintPure, Category = "HLW|Power")
	TArray<FName> GetGeneratorIDs() const;

	UFUNCTION(BlueprintPure, Category = "HLW|Power")
	AHLWPowerGenerator* FindGenerator(FName GeneratorID) const;

	// Switches on every circuit and starts every main generator. Faults stay.
	UFUNCTION(BlueprintCallable, Category = "HLW|Power")
	void RestoreFacilityPower();

	// Starts every backup generator
	UFUNCTION(BlueprintCallable, Category = "HLW|Power")
	void ActivateEmergencyPower();

	// Resolves now. Changes are otherwise batched to once per frame.
	UFUNCTION(BlueprintCallable, Category = "HLW|Power")
	void UpdatePowerAvailability();

	void RequestUpdate() { bUpdatePending = true; }

	// Called by generators on every state change
	void NotifyGeneratorStateChanged(AHLWPowerGenerator* Generator);

	void NotifyElectricalFault(FName FaultID, FName TargetID, EHLWElectricalFaultType FaultType, bool bActive);
	void NotifyFuseCollected(FName FuseType, AActor* Collector);

	// bInitial is true for the level's starting state, so UI and sounds can skip it
	UPROPERTY(BlueprintAssignable, Category = "HLW|Power")
	FHLWOnCircuitStateChanged OnCircuitStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "HLW|Power")
	FHLWOnGeneratorStateChanged OnGeneratorStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "HLW|Power")
	FHLWOnElectricalFault OnElectricalFault;

	UPROPERTY(BlueprintAssignable, Category = "HLW|Power")
	FHLWOnFuseCollected OnFuseCollected;

private:
	struct FCircuitEntry
	{
		FHLWCircuitData Data;
		EHLWPowerState State = EHLWPowerState::Unpowered;
		FName FeedingGenerator;
	};

	struct FResolveContext
	{
		TMap<FName, EHLWPowerState> States;
		TMap<FName, FName> Feeding;
		TSet<FName> Visiting;
	};

	EHLWPowerState ResolveCircuit(FName CircuitID, FResolveContext& Context) const;
	EHLWPowerState ResolveSupply(FName SourceID, FResolveContext& Context, FName& OutGenerator) const;

	// Fails generators over capacity. True if any failed.
	bool FailOverloadedGenerators(const FResolveContext& Context);

	void PushToConsumers(bool bInitial);
	void DrawDebug() const;

	UFUNCTION()
	void HandleStateRestored();

	TMap<FName, FCircuitEntry> Circuits;
	TMap<FName, bool> EnabledOverrides;
	TMap<FName, TSet<FName>> FaultSources;
	TMap<FName, TWeakObjectPtr<AHLWPowerGenerator>> Generators;
	TArray<TWeakObjectPtr<UHLWPowerConsumerComponent>> Consumers;

	bool bWorldStarted = false;
	bool bInitialResolveDone = false;
	bool bUpdatePending = false;
	bool bResolving = false;

	// Next resolve reports as initial, used after a load
	bool bNextResolveSilent = false;
};
