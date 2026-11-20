#pragma once

#include "CoreMinimal.h"
#include "UI/HLWFeedbackWidget.h"
#include "Electrical/HLWPowerTypes.h"
#include "HLWPowerStatusWidget.generated.h"

class UTextBlock;

// Base for generator and panel screens: lists generators and circuits with their state
UCLASS()
class THE_HOLLOW_API UHLWPowerStatusWidget : public UHLWFeedbackWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HLW|UI")
	void Refresh();

	// Empty lists show everything
	UFUNCTION(BlueprintCallable, Category = "HLW|UI")
	void SetFilter(const TArray<FName>& InGeneratorIDs, const TArray<FName>& InCircuitIDs);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void BuildDefaultLayout() override;

	UPROPERTY(BlueprintReadOnly, Category = "HLW|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(BlueprintReadOnly, Category = "HLW|UI", meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|UI")
	TArray<FName> GeneratorIDs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|UI")
	TArray<FName> CircuitIDs;

	// Keeps fuel readouts current
	UPROPERTY(EditAnywhere, Category = "HLW|UI", meta = (ClampMin = "0.1"))
	float RefreshInterval = 0.5f;

private:
	UFUNCTION()
	void HandleCircuitStateChanged(FName CircuitID, EHLWPowerState State, bool bInitial);

	UFUNCTION()
	void HandleGeneratorStateChanged(FName GeneratorID, EHLWGeneratorState State);

	FTimerHandle RefreshTimer;
};
