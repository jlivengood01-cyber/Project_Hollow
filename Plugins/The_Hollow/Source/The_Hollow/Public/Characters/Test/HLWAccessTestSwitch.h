#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HLWInteractableInterface.h"
#include "HLWAccessTestSwitch.generated.h"

class AHLWAccessDeviceBase;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EHLWTestSwitchMode : uint8
{
	Power,
	Network,
	Operational,
	AccessDisabled,
	Lockdown
};

// Test-only switch. Stand-in for the Electrical and Security systems.
UCLASS()
class THE_HOLLOW_API AHLWAccessTestSwitch : public AActor, public IHLWInteractableInterface
{
	GENERATED_BODY()

public:
	AHLWAccessTestSwitch();

	virtual void Interact_Implementation(AActor* InteractingActor) override;
	virtual FText GetInteractionText_Implementation() override;
	virtual bool CanInteract_Implementation() override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "HLW|Test")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, Category = "HLW|Test")
	EHLWTestSwitchMode Mode = EHLWTestSwitchMode::Power;

	// Readers, elevator panels or terminals. AccessDisabled affects readers only. Not used in Lockdown mode.
	UPROPERTY(EditInstanceOnly, Category = "HLW|Test", meta = (EditCondition = "Mode != EHLWTestSwitchMode::Lockdown"))
	TArray<TObjectPtr<AHLWAccessDeviceBase>> Devices;

	UPROPERTY(EditAnywhere, Category = "HLW|Test", meta = (EditCondition = "Mode == EHLWTestSwitchMode::Lockdown"))
	FName LockdownZone;

private:
	// True when the toggled condition is on (powered, networked, working, disabled, locked down)
	bool IsActive() const;
};
