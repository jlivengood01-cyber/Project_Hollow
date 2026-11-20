#pragma once

#include "CoreMinimal.h"
#include "Electrical/HLWPoweredActor.h"
#include "HLWElectricalHazard.generated.h"

class UAudioComponent;
class UBoxComponent;
class UPointLightComponent;
class USoundBase;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHLWOnHazardTouched, AActor*, Victim);

// Live while its circuit is powered. Touching it while live only broadcasts, no damage yet.
UCLASS()
class THE_HOLLOW_API AHLWElectricalHazardTrigger : public AHLWPoweredActor
{
	GENERATED_BODY()

public:
	AHLWElectricalHazardTrigger();

	UFUNCTION(BlueprintPure, Category = "HLW|Hazard")
	bool IsLive() const { return IsPowered(); }

	UPROPERTY(BlueprintAssignable, Category = "HLW|Hazard")
	FHLWOnHazardTouched OnHazardTouched;

protected:
	virtual void BeginPlay() override;
	virtual void ApplyPower(bool bPowered, bool bInitial) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Hazard")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Hazard")
	TObjectPtr<UAudioComponent> Audio;

	// Looping buzz while live
	UPROPERTY(EditAnywhere, Category = "HLW|Hazard|Feedback")
	TObjectPtr<USoundBase> LiveLoopSound;

	UPROPERTY(EditAnywhere, Category = "HLW|Hazard|Feedback")
	bool bShowDebugResult = true;

	UFUNCTION(BlueprintImplementableEvent, Category = "HLW|Hazard")
	void OnHazardLiveChanged(bool bLive, bool bInitial);

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void Touch(AActor* Victim);
};

// Loose cables that are live with their circuit
UCLASS()
class THE_HOLLOW_API AHLWExposedWiring : public AHLWElectricalHazardTrigger
{
	GENERATED_BODY()

public:
	AHLWExposedWiring();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Hazard")
	TObjectPtr<UStaticMeshComponent> Mesh;
};

// Panel that sparks while its circuit is live
UCLASS()
class THE_HOLLOW_API AHLWSparkingPanel : public AHLWElectricalHazardTrigger
{
	GENERATED_BODY()

public:
	AHLWSparkingPanel();

protected:
	virtual void ApplyPower(bool bPowered, bool bInitial) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Hazard")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Hazard")
	TObjectPtr<UPointLightComponent> SparkLight;

	UPROPERTY(EditAnywhere, Category = "HLW|Hazard|Feedback", meta = (ClampMin = "0"))
	float SparkIntensity = 3000.f;

private:
	void Spark();

	FTimerHandle SparkTimer;
};
