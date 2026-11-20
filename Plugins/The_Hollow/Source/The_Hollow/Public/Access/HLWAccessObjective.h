#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Access/HLWAccessTypes.h"
#include "HLWAccessObjective.generated.h"

class AHLWKeycardReader;
class UHLWKeycardDefinition;

UENUM(BlueprintType)
enum class EHLWAccessObjectiveStage : uint8
{
	Inactive,
	FindCredential,
	ReturnToDevice,
	Complete
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHLWOnObjectiveStageChanged, EHLWAccessObjectiveStage, Stage);

// Placeholder access objective until SYS_PLAYER_OBJECTIVE_001 exists.
// Denied at the reader > find the card > return > granted.
UCLASS()
class THE_HOLLOW_API AHLWAccessObjective : public AActor
{
	GENERATED_BODY()

public:
	AHLWAccessObjective();

	UFUNCTION(BlueprintPure, Category = "HLW|Objective")
	EHLWAccessObjectiveStage GetStage() const { return Stage; }

	UFUNCTION(BlueprintPure, Category = "HLW|Objective")
	FText GetStageText() const;

	UPROPERTY(BlueprintAssignable, Category = "HLW|Objective")
	FHLWOnObjectiveStageChanged OnStageChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Objective")
	FText ObjectiveName;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "HLW|Objective")
	TObjectPtr<AHLWKeycardReader> TargetReader;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Objective")
	TObjectPtr<UHLWKeycardDefinition> RequiredKeycard;

	// Off waits for the first denied scan at the target reader
	UPROPERTY(EditAnywhere, Category = "HLW|Objective")
	bool bStartActive = false;

	UPROPERTY(EditAnywhere, Category = "HLW|Objective|Text")
	FText FindCredentialText;

	UPROPERTY(EditAnywhere, Category = "HLW|Objective|Text")
	FText ReturnToDeviceText;

	UPROPERTY(EditAnywhere, Category = "HLW|Objective|Text")
	FText CompleteText;

	// Prints stage changes on screen until objective UI exists
	UPROPERTY(EditAnywhere, Category = "HLW|Objective")
	bool bShowDebugText = true;

private:
	UFUNCTION()
	void HandleAccessResolved(FName DeviceID, EHLWAccessResult Result);

	UFUNCTION()
	void HandleKeycardCollected(UHLWKeycardDefinition* Keycard, AActor* Collector);

	UFUNCTION()
	void HandleStateRestored();

	void SetStage(EHLWAccessObjectiveStage NewStage, bool bAnnounce);

	FName ObjectiveID;
	EHLWAccessObjectiveStage Stage = EHLWAccessObjectiveStage::Inactive;
};
