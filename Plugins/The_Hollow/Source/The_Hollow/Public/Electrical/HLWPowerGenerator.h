#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HLWInteractableInterface.h"
#include "Electrical/HLWPowerTypes.h"
#include "HLWPowerGenerator.generated.h"

class UAudioComponent;
class UHLWElectricalFaultComponent;
class UPointLightComponent;
class USoundBase;
class UStaticMeshComponent;

// Main or backup power source. Circuits name it by GeneratorID.
UCLASS()
class THE_HOLLOW_API AHLWPowerGenerator : public AActor, public IHLWInteractableInterface
{
	GENERATED_BODY()

public:
	AHLWPowerGenerator();

	// False if failed, out of fuel or already running
	UFUNCTION(BlueprintCallable, Category = "HLW|Generator")
	bool StartGenerator();

	UFUNCTION(BlueprintCallable, Category = "HLW|Generator")
	void StopGenerator();

	// Not failed and has fuel if it needs it
	UFUNCTION(BlueprintPure, Category = "HLW|Generator")
	bool CheckGeneratorCondition() const;

	// 0 when not running, Capacity when running, -1 when running with unlimited capacity
	UFUNCTION(BlueprintPure, Category = "HLW|Generator")
	float GetPowerOutput() const;

	UFUNCTION(BlueprintCallable, Category = "HLW|Generator")
	void TriggerGeneratorFailure();

	// Clears a failure, leaves it off. Faults on a fault component are repaired through it.
	UFUNCTION(BlueprintCallable, Category = "HLW|Generator")
	void RepairGenerator();

	UFUNCTION(BlueprintCallable, Category = "HLW|Generator")
	void AddFuel(float Amount);

	UFUNCTION(BlueprintPure, Category = "HLW|Generator")
	bool IsRunning() const { return State == EHLWGeneratorState::Running; }

	UFUNCTION(BlueprintPure, Category = "HLW|Generator")
	EHLWGeneratorState GetGeneratorState() const { return State; }

	// The set ID, or the actor's stable ID when empty
	UFUNCTION(BlueprintPure, Category = "HLW|Generator")
	FName GetGeneratorID() const;

	UFUNCTION(BlueprintPure, Category = "HLW|Generator")
	float GetFuel() const { return Fuel; }

	const FHLWGeneratorData& GetGeneratorData() const { return GeneratorData; }

	virtual void Interact_Implementation(AActor* InteractingActor) override;
	virtual FText GetInteractionText_Implementation() override;
	virtual bool CanInteract_Implementation() override;

	UPROPERTY(BlueprintAssignable, Category = "HLW|Generator")
	FHLWOnGeneratorStateChanged OnStateChanged;

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "HLW|Generator")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "HLW|Generator")
	TObjectPtr<UPointLightComponent> StatusLight;

	// Plays RunningSound while running
	UPROPERTY(VisibleAnywhere, Category = "HLW|Generator")
	TObjectPtr<UAudioComponent> Audio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Generator")
	FHLWGeneratorData GeneratorData;

	// Off for generators driven only by a control panel or script
	UPROPERTY(EditAnywhere, Category = "HLW|Generator")
	bool bPlayerCanOperate = true;

	// Interact repairs a failed generator. A fault component on this actor sets the requirements.
	UPROPERTY(EditAnywhere, Category = "HLW|Generator")
	bool bPlayerCanRepair = true;

	UPROPERTY(EditAnywhere, Category = "HLW|Generator|Feedback")
	TObjectPtr<USoundBase> StartSound;

	// Should be a looping sound
	UPROPERTY(EditAnywhere, Category = "HLW|Generator|Feedback")
	TObjectPtr<USoundBase> RunningSound;

	UPROPERTY(EditAnywhere, Category = "HLW|Generator|Feedback")
	TObjectPtr<USoundBase> StopSound;

	UPROPERTY(EditAnywhere, Category = "HLW|Generator|Feedback")
	TObjectPtr<USoundBase> FailSound;

	UPROPERTY(EditAnywhere, Category = "HLW|Generator|Feedback")
	FLinearColor StartingColor = FLinearColor(1.f, 0.45f, 0.f);

	UPROPERTY(EditAnywhere, Category = "HLW|Generator|Feedback")
	FLinearColor RunningColor = FLinearColor(0.1f, 1.f, 0.2f);

	UPROPERTY(EditAnywhere, Category = "HLW|Generator|Feedback")
	FLinearColor FailedColor = FLinearColor(1.f, 0.05f, 0.05f);

	UPROPERTY(EditAnywhere, Category = "HLW|Generator|Feedback", meta = (ClampMin = "0"))
	float LightIntensity = 800.f;

	UPROPERTY(EditAnywhere, Category = "HLW|Generator|Feedback")
	bool bShowDebugResult = true;

private:
	UFUNCTION()
	void HandleStateRestored();

	// Saved or editor starting state, no sounds or reports
	void LoadStartingState();
	void SetState(EHLWGeneratorState NewState);
	void StartStateTimers();
	void SaveState() const;
	UHLWElectricalFaultComponent* GetActiveFault() const;
	void FinishStartup();
	void DrainFuel();
	void UpdateFeedback();
	void PlayOneShot(USoundBase* Sound) const;
	void Report(const FString& Message) const;

	UPROPERTY(VisibleInstanceOnly, Category = "HLW|Generator")
	EHLWGeneratorState State = EHLWGeneratorState::Off;

	UPROPERTY(VisibleInstanceOnly, Category = "HLW|Generator")
	float Fuel = 0.f;

	FTimerHandle StartupTimer;
	FTimerHandle FuelTimer;
	FTimerHandle RunTimer;
};
