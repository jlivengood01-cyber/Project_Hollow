#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Electrical/HLWPowerTypes.h"
#include "HLWPoweredActor.generated.h"

class UHLWPowerConsumerComponent;

// Base for equipment that follows its circuit. Set the circuit on the Power component.
UCLASS(Abstract)
class THE_HOLLOW_API AHLWPoweredActor : public AActor
{
	GENERATED_BODY()

public:
	AHLWPoweredActor();

	UFUNCTION(BlueprintPure, Category = "HLW|Power")
	bool IsPowered() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// bInitial is the starting state or a load, so skip sounds and effects
	virtual void ApplyPower(bool bPowered, bool bInitial) {}
	virtual void ApplyCircuitState(EHLWPowerState State, bool bInitial) {}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Power")
	TObjectPtr<UHLWPowerConsumerComponent> Power;

private:
	UFUNCTION()
	void HandlePowerChanged(bool bPowered, bool bInitial);

	UFUNCTION()
	void HandleCircuitStateChanged(EHLWPowerState State, bool bInitial);
};
