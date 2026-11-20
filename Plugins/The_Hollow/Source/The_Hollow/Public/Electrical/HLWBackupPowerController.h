#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Electrical/HLWPowerTypes.h"
#include "HLWBackupPowerController.generated.h"

class UBillboardComponent;

// Starts the backup generator when main power fails and stops it when main power returns.
// Circuits with a BackupSourceID then run emergency loads only.
UCLASS()
class THE_HOLLOW_API AHLWBackupPowerController : public AActor
{
	GENERATED_BODY()

public:
	AHLWBackupPowerController();

	UFUNCTION(BlueprintPure, Category = "HLW|Backup")
	bool IsMainPowerDown() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Empty watches every generator not marked as backup
	UPROPERTY(EditAnywhere, Category = "HLW|Backup")
	TArray<FName> MainGeneratorIDs;

	// Empty starts every generator marked as backup
	UPROPERTY(EditAnywhere, Category = "HLW|Backup")
	TArray<FName> BackupGeneratorIDs;

	UPROPERTY(EditAnywhere, Category = "HLW|Backup", meta = (ClampMin = "0"))
	float StartDelay = 3.f;

	// Off means only a failure or running out of fuel triggers backup, not switching main off
	UPROPERTY(EditAnywhere, Category = "HLW|Backup")
	bool bStartOnManualStop = false;

	UPROPERTY(EditAnywhere, Category = "HLW|Backup")
	bool bStopWhenMainReturns = true;

	UPROPERTY(EditAnywhere, Category = "HLW|Backup")
	bool bShowDebugResult = true;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Sprite;
#endif

private:
	UFUNCTION()
	void HandleGeneratorStateChanged(FName GeneratorID, EHLWGeneratorState State);

	void Evaluate();
	void StartBackup();
	void StopBackup();
	bool IsMainGenerator(FName GeneratorID) const;
	TArray<FName> GetMainIDs() const;
	TArray<FName> GetBackupIDs() const;
	void Report(const FString& Message) const;

	FTimerHandle StartTimer;
};
