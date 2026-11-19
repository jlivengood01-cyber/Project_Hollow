#pragma once

#include "CoreMinimal.h"
#include "Access/HLWAccessDeviceBase.h"
#include "Access/HLWAccessTypes.h"
#include "HLWInteractableInterface.h"
#include "HLWElevatorAccessPanel.generated.h"

USTRUCT(BlueprintType)
struct FHLWElevatorFloor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Elevator")
	FText FloorName;

	// Unrestricted floors need no card
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Elevator")
	bool bRestricted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Elevator", meta = (EditCondition = "bRestricted"))
	FHLWFacilityAccessData Access;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHLWOnFloorAuthorized, int32, FloorIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHLWOnFloorDenied, int32, FloorIndex, EHLWAccessResult, Result);

// Floor authorization for an elevator. Moving the car belongs to SYS_ELEVATOR_001.
UCLASS()
class THE_HOLLOW_API AHLWElevatorAccessPanel : public AHLWAccessDeviceBase, public IHLWInteractableInterface
{
	GENERATED_BODY()

public:
	AHLWElevatorAccessPanel();

	// No broadcast, for greying out floor buttons
	UFUNCTION(BlueprintPure, Category = "HLW|Elevator")
	EHLWAccessResult CheckFloor(int32 FloorIndex, AActor* User) const;

	UFUNCTION(BlueprintCallable, Category = "HLW|Elevator")
	EHLWAccessResult RequestFloor(int32 FloorIndex, AActor* User);

	UFUNCTION(BlueprintPure, Category = "HLW|Elevator")
	int32 GetCurrentFloor() const { return CurrentFloor; }

	// For the Elevator System to report arrival
	UFUNCTION(BlueprintCallable, Category = "HLW|Elevator")
	void SetCurrentFloor(int32 FloorIndex);

	UPROPERTY(BlueprintAssignable, Category = "HLW|Elevator")
	FHLWOnFloorAuthorized OnFloorAuthorized;

	UPROPERTY(BlueprintAssignable, Category = "HLW|Elevator")
	FHLWOnFloorDenied OnFloorDenied;

	// Interact requests the next floor, a test stand-in for the floor buttons
	virtual void Interact_Implementation(AActor* InteractingActor) override;
	virtual FText GetInteractionText_Implementation() override;
	virtual bool CanInteract_Implementation() override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Elevator")
	TArray<FHLWElevatorFloor> Floors;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Elevator", meta = (ClampMin = "0"))
	int32 CurrentFloor = 0;

	// Instant arrival on success until the Elevator System moves the car
	UPROPERTY(EditAnywhere, Category = "HLW|Elevator")
	bool bArriveInstantly = true;

	UPROPERTY(EditAnywhere, Category = "HLW|Elevator")
	bool bShowDebugResult = true;

private:
	EHLWAccessResult EvaluateFloor(int32 FloorIndex, AActor* User, bool bBroadcast) const;
	int32 GetNextFloor() const;
};
