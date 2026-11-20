#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HLWInteractableInterface.h"
#include "Electrical/HLWPowerTypes.h"
#include "HLWCircuitSwitch.generated.h"

class UHLWElectricalFaultComponent;
class UHLWFacilityPowerSubsystem;
class UPointLightComponent;
class USoundBase;
class UStaticMeshComponent;
struct FHLWSwitchSaveData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHLWOnSwitched, bool, bOn);

// Switches one circuit on and off. Never produces power itself.
// Light: red off, green live, amber on with no incoming power.
UCLASS(Abstract)
class THE_HOLLOW_API AHLWCircuitSwitch : public AActor, public IHLWInteractableInterface
{
	GENERATED_BODY()

public:
	AHLWCircuitSwitch();

	// False if this switch can't be turned on right now
	UFUNCTION(BlueprintCallable, Category = "HLW|Switch")
	bool SetSwitchOn(bool bNewOn);

	UFUNCTION(BlueprintPure, Category = "HLW|Switch")
	bool IsSwitchOn() const { return bOn; }

	UFUNCTION(BlueprintPure, Category = "HLW|Switch")
	FName GetCircuitID() const { return CircuitID; }

	virtual void Interact_Implementation(AActor* InteractingActor) override;
	virtual FText GetInteractionText_Implementation() override;
	virtual bool CanInteract_Implementation() override;

	UPROPERTY(BlueprintAssignable, Category = "HLW|Switch")
	FHLWOnSwitched OnSwitched;

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual bool CanSwitchOn() const { return true; }

	// Null Saved means editor defaults
	virtual void ReadSaveData(const FHLWSwitchSaveData* Saved);
	virtual void WriteSaveData(FHLWSwitchSaveData& Out) const;

	// Saved or editor state, applied without sounds
	virtual void LoadStartingState();

	void SaveState() const;

	// Pushes the switch position to the circuit
	void ApplyToCircuit() const;
	void UpdateLight();
	void PlayOneShot(USoundBase* Sound) const;
	void Report(const FString& Message) const;
	FText GetCircuitName() const;
	UHLWFacilityPowerSubsystem* GetPowerSubsystem() const;
	FName GetSwitchID() const;
	UHLWElectricalFaultComponent* GetActiveFault() const;

	UPROPERTY(VisibleAnywhere, Category = "HLW|Switch")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "HLW|Switch")
	TObjectPtr<UPointLightComponent> StatusLight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Switch")
	FName CircuitID;

	UPROPERTY(EditAnywhere, Category = "HLW|Switch")
	bool bStartsOn = false;

	UPROPERTY(EditAnywhere, Category = "HLW|Switch|Feedback")
	TObjectPtr<USoundBase> SwitchOnSound;

	UPROPERTY(EditAnywhere, Category = "HLW|Switch|Feedback")
	TObjectPtr<USoundBase> SwitchOffSound;

	UPROPERTY(EditAnywhere, Category = "HLW|Switch|Feedback")
	FLinearColor OffColor = FLinearColor(1.f, 0.05f, 0.05f);

	UPROPERTY(EditAnywhere, Category = "HLW|Switch|Feedback")
	FLinearColor LiveColor = FLinearColor(0.1f, 1.f, 0.2f);

	UPROPERTY(EditAnywhere, Category = "HLW|Switch|Feedback")
	FLinearColor NoPowerColor = FLinearColor(1.f, 0.45f, 0.f);

	UPROPERTY(EditAnywhere, Category = "HLW|Switch|Feedback", meta = (ClampMin = "0"))
	float LightIntensity = 300.f;

	UPROPERTY(EditAnywhere, Category = "HLW|Switch|Feedback")
	bool bShowDebugResult = true;

	UPROPERTY(VisibleInstanceOnly, Category = "HLW|Switch")
	bool bOn = false;

private:
	UFUNCTION()
	void HandleCircuitStateChanged(FName ChangedCircuitID, EHLWPowerState State, bool bInitial);

	UFUNCTION()
	void HandleStateRestored();
};

// Plain flip switch. Several in one Blueprint make an electrical panel.
UCLASS()
class THE_HOLLOW_API AHLWCircuitBreaker : public AHLWCircuitSwitch
{
	GENERATED_BODY()
};
