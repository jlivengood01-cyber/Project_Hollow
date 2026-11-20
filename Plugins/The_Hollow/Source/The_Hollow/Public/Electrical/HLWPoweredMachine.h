#pragma once

#include "CoreMinimal.h"
#include "Electrical/HLWPoweredActor.h"
#include "HLWInteractableInterface.h"
#include "HLWPoweredMachine.generated.h"

class UAudioComponent;
class URotatingMovementComponent;
class USoundBase;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHLWOnMachineInspected, bool, bPowered);

// Placeholder machinery. Loops its running sound while powered.
UCLASS()
class THE_HOLLOW_API AHLWPoweredMachine : public AHLWPoweredActor, public IHLWInteractableInterface
{
	GENERATED_BODY()

public:
	AHLWPoweredMachine();

	virtual void Interact_Implementation(AActor* InteractingActor) override;
	virtual FText GetInteractionText_Implementation() override;
	virtual bool CanInteract_Implementation() override;

	// Objectives use this to confirm equipment is running
	UPROPERTY(BlueprintAssignable, Category = "HLW|Machine")
	FHLWOnMachineInspected OnInspected;

protected:
	virtual void ApplyPower(bool bPowered, bool bInitial) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Machine")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Machine")
	TObjectPtr<UAudioComponent> Audio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Machine")
	FText DisplayName;

	// Off hides the interaction prompt
	UPROPERTY(EditAnywhere, Category = "HLW|Machine")
	bool bCanInspect = true;

	// Should be a looping sound
	UPROPERTY(EditAnywhere, Category = "HLW|Machine|Feedback")
	TObjectPtr<USoundBase> RunningSound;

	UPROPERTY(EditAnywhere, Category = "HLW|Machine|Feedback")
	TObjectPtr<USoundBase> StartSound;

	UPROPERTY(EditAnywhere, Category = "HLW|Machine|Feedback")
	TObjectPtr<USoundBase> StopSound;

	UPROPERTY(EditAnywhere, Category = "HLW|Machine|Feedback")
	bool bShowDebugResult = true;

	UFUNCTION(BlueprintImplementableEvent, Category = "HLW|Machine")
	void OnMachinePowerChanged(bool bPowered, bool bInitial);

	FText GetMachineName() const;
};

// Placeholder vent with a fan that spins while powered
UCLASS()
class THE_HOLLOW_API AHLWPoweredVentilation : public AHLWPoweredMachine
{
	GENERATED_BODY()

public:
	AHLWPoweredVentilation();

protected:
	virtual void ApplyPower(bool bPowered, bool bInitial) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Machine")
	TObjectPtr<UStaticMeshComponent> Fan;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Machine")
	TObjectPtr<URotatingMovementComponent> FanSpin;
};
