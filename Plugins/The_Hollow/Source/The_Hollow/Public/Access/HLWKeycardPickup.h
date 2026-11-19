#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Access/HLWAccessTypes.h"
#include "HLWInteractableInterface.h"
#include "HLWKeycardPickup.generated.h"

class UHLWKeycardDefinition;
class USoundBase;
class UStaticMeshComponent;

// World pickup that adds its keycard to the collector's inventory
UCLASS()
class THE_HOLLOW_API AHLWKeycardPickup : public AActor, public IHLWInteractableInterface
{
	GENERATED_BODY()

public:
	AHLWKeycardPickup();

	// True once the inventory has accepted the card
	UFUNCTION(BlueprintCallable, Category = "HLW|Keycard")
	bool CollectKeycard(AActor* Collector);

	UFUNCTION(BlueprintPure, Category = "HLW|Keycard")
	bool IsCollected() const { return bCollected; }

	UPROPERTY(BlueprintAssignable, Category = "HLW|Keycard")
	FHLWOnKeycardCollected OnCollected;

	virtual void Interact_Implementation(AActor* InteractingActor) override;
	virtual FText GetInteractionText_Implementation() override;
	virtual bool CanInteract_Implementation() override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Keycard")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Keycard")
	TObjectPtr<UHLWKeycardDefinition> Keycard;

	UPROPERTY(EditAnywhere, Category = "HLW|Keycard")
	TObjectPtr<USoundBase> PickupSound;

private:
	UFUNCTION()
	void HandleStateRestored();

	void SetCollectedState(bool bNewCollected);

	FName PickupID;
	bool bCollected = false;
};
