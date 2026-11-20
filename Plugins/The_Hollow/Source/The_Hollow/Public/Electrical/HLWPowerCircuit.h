#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Electrical/HLWPowerTypes.h"
#include "HLWPowerCircuit.generated.h"

class UBillboardComponent;
class UTextRenderComponent;

// Placed circuit node. Child circuits name this one as their SourceID, which covers junctions and relays.
UCLASS()
class THE_HOLLOW_API AHLWPowerCircuit : public AActor
{
	GENERATED_BODY()

public:
	AHLWPowerCircuit();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "HLW|Circuit")
	void EnableCircuit();

	UFUNCTION(BlueprintCallable, Category = "HLW|Circuit")
	void DisableCircuit();

	UFUNCTION(BlueprintCallable, Category = "HLW|Circuit")
	void SetCircuitFaulted(bool bFaulted);

	UFUNCTION(BlueprintPure, Category = "HLW|Circuit")
	EHLWPowerState GetCircuitPowerStatus() const;

	// The set ID, or the actor's stable ID when empty
	UFUNCTION(BlueprintPure, Category = "HLW|Circuit")
	FName GetCircuitID() const;

protected:
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Circuit")
	FHLWCircuitData CircuitData;

	// Editor-only label showing the ID and source
	UPROPERTY(VisibleAnywhere, Category = "HLW|Circuit")
	TObjectPtr<UTextRenderComponent> Label;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Sprite;
#endif
};
