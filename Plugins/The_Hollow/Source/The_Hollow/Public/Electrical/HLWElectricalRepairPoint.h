#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HLWInteractableInterface.h"
#include "HLWElectricalRepairPoint.generated.h"

class UAudioComponent;
class UHLWElectricalFaultComponent;
class UPointLightComponent;
class USoundBase;
class UStaticMeshComponent;

// Damaged wiring or relay in a wall. Its fault cuts a circuit until repaired here.
// Set Target ID on the Fault component to the circuit it cuts.
UCLASS()
class THE_HOLLOW_API AHLWElectricalRepairPoint : public AActor, public IHLWInteractableInterface
{
	GENERATED_BODY()

public:
	AHLWElectricalRepairPoint();

	virtual void Interact_Implementation(AActor* InteractingActor) override;
	virtual FText GetInteractionText_Implementation() override;
	virtual bool CanInteract_Implementation() override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Fault")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Fault")
	TObjectPtr<UHLWElectricalFaultComponent> Fault;

	// Flickers while the fault is active
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Fault")
	TObjectPtr<UPointLightComponent> SparkLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Fault")
	TObjectPtr<UAudioComponent> Audio;

	// Should be a looping sound
	UPROPERTY(EditAnywhere, Category = "HLW|Fault|Feedback")
	TObjectPtr<USoundBase> DamagedLoopSound;

	UPROPERTY(EditAnywhere, Category = "HLW|Fault|Feedback", meta = (ClampMin = "0"))
	float SparkIntensity = 3000.f;

	UFUNCTION(BlueprintImplementableEvent, Category = "HLW|Fault")
	void OnFaultStateChanged(bool bActive);

private:
	UFUNCTION()
	void HandleFaultChanged(bool bActive);

	void Spark();

	FTimerHandle SparkTimer;
};
