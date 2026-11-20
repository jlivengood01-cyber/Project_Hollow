#pragma once

#include "CoreMinimal.h"
#include "Electrical/HLWPoweredActor.h"
#include "HLWPoweredLight.generated.h"

class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;

// Placeholder ceiling light that follows its circuit.
// Flickers when power returns. Tick Emergency Load on the Power component for an emergency light.
UCLASS()
class THE_HOLLOW_API AHLWPoweredLight : public AHLWPoweredActor
{
	GENERATED_BODY()

public:
	AHLWPoweredLight();

protected:
	virtual void BeginPlay() override;
	virtual void ApplyPower(bool bPowered, bool bInitial) override;
	virtual void ApplyCircuitState(EHLWPowerState State, bool bInitial) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Light")
	TObjectPtr<UStaticMeshComponent> Fixture;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Light")
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY(EditAnywhere, Category = "HLW|Light|Flicker")
	bool bFlickerOnRestore = true;

	// On/off toggles before settling on
	UPROPERTY(EditAnywhere, Category = "HLW|Light|Flicker", meta = (ClampMin = "1", EditCondition = "bFlickerOnRestore"))
	int32 FlickerCount = 5;

	UPROPERTY(EditAnywhere, Category = "HLW|Light|Flicker", meta = (ClampMin = "0.01", EditCondition = "bFlickerOnRestore"))
	float FlickerMinInterval = 0.04f;

	UPROPERTY(EditAnywhere, Category = "HLW|Light|Flicker", meta = (ClampMin = "0.01", EditCondition = "bFlickerOnRestore"))
	float FlickerMaxInterval = 0.15f;

	// Scalar parameter on the fixture material. None skips it.
	UPROPERTY(EditAnywhere, Category = "HLW|Light|Material")
	FName EmissiveParameter = TEXT("Emissive");

	UPROPERTY(EditAnywhere, Category = "HLW|Light|Material")
	float EmissiveOn = 10.f;

	UPROPERTY(EditAnywhere, Category = "HLW|Light|Material")
	float EmissiveOff = 0.f;

	// Color while the circuit runs on backup power
	UPROPERTY(EditAnywhere, Category = "HLW|Light|Emergency")
	bool bTintOnBackup = true;

	UPROPERTY(EditAnywhere, Category = "HLW|Light|Emergency", meta = (EditCondition = "bTintOnBackup"))
	FLinearColor BackupColor = FLinearColor(1.f, 0.1f, 0.05f);

	// Blueprint hook for extra effects
	UFUNCTION(BlueprintImplementableEvent, Category = "HLW|Light")
	void OnLightPowerChanged(bool bPowered, bool bInitial);

private:
	void SetLightOn(bool bOn);
	void FlickerStep();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FixtureMaterial;

	FLinearColor NormalColor = FLinearColor::White;
	FTimerHandle FlickerTimer;
	int32 FlickerStepsLeft = 0;
};
