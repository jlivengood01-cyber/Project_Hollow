#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Access/HLWAccessDeviceState.h"
#include "HLWAccessDeviceBase.generated.h"

class UHLWPowerConsumerComponent;
class UStaticMeshComponent;

// Shared power, network and fault state for readers, elevator panels and terminals
UCLASS(Abstract)
class THE_HOLLOW_API AHLWAccessDeviceBase : public AActor, public IHLWAccessDeviceState
{
	GENERATED_BODY()

public:
	AHLWAccessDeviceBase();

	// Driven by the PowerConsumer when it has a circuit, otherwise set by hand
	UFUNCTION(BlueprintCallable, Category = "HLW|Device")
	void SetPowered(bool bNewPowered);

	UFUNCTION(BlueprintCallable, Category = "HLW|Device")
	void SetNetworked(bool bNewNetworked);

	UFUNCTION(BlueprintCallable, Category = "HLW|Device")
	void SetOperational(bool bNewOperational);

	virtual bool IsPowered_Implementation() override { return bPowered; }
	virtual bool IsNetworked_Implementation() override { return bNetworked; }
	virtual bool IsOperational_Implementation() override { return bOperational; }

protected:
	virtual void BeginPlay() override;

	// Called after any of the three states changes
	virtual void OnDeviceStateChanged(bool bPowerChanged) {}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Device")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// Set a circuit to tie this device to the electrical system. Empty keeps bPowered manual.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Device")
	TObjectPtr<UHLWPowerConsumerComponent> PowerConsumer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Device|State")
	bool bPowered = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Device|State")
	bool bNetworked = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Device|State")
	bool bOperational = true;

private:
	UFUNCTION()
	void HandlePowerChanged(bool bNewPowered, bool bInitial);
};
