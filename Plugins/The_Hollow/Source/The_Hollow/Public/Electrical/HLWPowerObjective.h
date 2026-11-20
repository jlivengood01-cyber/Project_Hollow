#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Electrical/HLWPowerTypes.h"
#include "HLWPowerObjective.generated.h"

class AHLWFusebox;
class AHLWPowerGenerator;
class AHLWPoweredMachine;

UENUM(BlueprintType)
enum class EHLWPowerObjectiveStage : uint8
{
	Inactive,
	FindGenerator    UMETA(DisplayName = "Find Generator"),
	InspectPanel     UMETA(DisplayName = "Inspect Panel"),
	FindFuse         UMETA(DisplayName = "Find Fuse"),
	InstallFuse      UMETA(DisplayName = "Install Fuse"),
	ActivateFusebox  UMETA(DisplayName = "Activate Fusebox"),
	RestoreCircuit   UMETA(DisplayName = "Restore Circuit"),
	ConfirmEquipment UMETA(DisplayName = "Confirm Equipment"),
	Complete
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHLWOnPowerObjectiveStageChanged, EHLWPowerObjectiveStage, Stage);

// Placeholder "Restore Power" objective until SYS_PLAYER_OBJECTIVE_001 exists.
// Only moves forward, and skips stages that are already done.
UCLASS()
class THE_HOLLOW_API AHLWPowerObjective : public AActor
{
	GENERATED_BODY()

public:
	AHLWPowerObjective();

	UFUNCTION(BlueprintPure, Category = "HLW|Objective")
	EHLWPowerObjectiveStage GetStage() const { return Stage; }

	UFUNCTION(BlueprintPure, Category = "HLW|Objective")
	FText GetStageText() const;

	// Starts the objective if it is inactive
	UFUNCTION(BlueprintCallable, Category = "HLW|Objective")
	void Activate();

	UPROPERTY(BlueprintAssignable, Category = "HLW|Objective")
	FHLWOnPowerObjectiveStageChanged OnStageChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Objective")
	FText ObjectiveName;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "HLW|Objective")
	TObjectPtr<AHLWPowerGenerator> TargetGenerator;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "HLW|Objective")
	TObjectPtr<AHLWFusebox> TargetFusebox;

	// Empty uses the fusebox's circuit
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Objective")
	FName TargetCircuitID;

	// Inspected while powered to finish. Empty finishes when the circuit is restored.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "HLW|Objective")
	TObjectPtr<AHLWPoweredMachine> ConfirmEquipment;

	// Off waits for Activate
	UPROPERTY(EditAnywhere, Category = "HLW|Objective")
	bool bStartActive = true;

	UPROPERTY(EditAnywhere, Category = "HLW|Objective|Text")
	TMap<EHLWPowerObjectiveStage, FText> StageTexts;

	// Prints stage changes on screen until objective UI exists
	UPROPERTY(EditAnywhere, Category = "HLW|Objective")
	bool bShowDebugText = true;

private:
	UFUNCTION()
	void HandleGeneratorStateChanged(FName GeneratorID, EHLWGeneratorState State);

	UFUNCTION()
	void HandleCircuitStateChanged(FName CircuitID, EHLWPowerState State, bool bInitial);

	UFUNCTION()
	void HandleFuseCollected(FName FuseType, AActor* Collector);

	UFUNCTION()
	void HandleFuseboxInspected();

	UFUNCTION()
	void HandleFuseInstalled(FName FuseType);

	UFUNCTION()
	void HandleFuseboxSwitched(bool bOn);

	UFUNCTION()
	void HandleEquipmentInspected(bool bPowered);

	UFUNCTION()
	void HandleStateRestored();

	UFUNCTION()
	void Advance();

	// Whether the current stage's goal is met right now
	bool IsStageDone(EHLWPowerObjectiveStage Check) const;
	FName GetCircuitID() const;
	void SetStage(EHLWPowerObjectiveStage NewStage, bool bAnnounce);

	FName ObjectiveID;
	EHLWPowerObjectiveStage Stage = EHLWPowerObjectiveStage::Inactive;
	bool bEquipmentConfirmed = false;
};
