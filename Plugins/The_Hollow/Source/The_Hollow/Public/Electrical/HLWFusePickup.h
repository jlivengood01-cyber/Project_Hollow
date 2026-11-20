#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HLWInteractableInterface.h"
#include "HLWFusePickup.generated.h"

class UHLWFuseDefinition;
class USoundBase;
class UStaticMeshComponent;

// Fuse on the floor. Goes into the collector's item holder.
UCLASS()
class THE_HOLLOW_API AHLWFusePickup : public AActor, public IHLWInteractableInterface
{
	GENERATED_BODY()

public:
	AHLWFusePickup();

	UFUNCTION(BlueprintCallable, Category = "HLW|Fuse")
	bool CollectFuse(AActor* Collector);

	virtual void Interact_Implementation(AActor* InteractingActor) override;
	virtual FText GetInteractionText_Implementation() override;
	virtual bool CanInteract_Implementation() override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "HLW|Fuse")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Fuse")
	TObjectPtr<UHLWFuseDefinition> Fuse;

	UPROPERTY(EditAnywhere, Category = "HLW|Fuse")
	TObjectPtr<USoundBase> PickupSound;

private:
	UFUNCTION()
	void HandleStateRestored();

	// Hidden rather than destroyed so a load can bring it back
	void SetCollectedState(bool bNewCollected);

	FName PickupID;
	bool bCollected = false;
};
