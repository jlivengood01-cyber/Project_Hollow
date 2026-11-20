#pragma once

#include "CoreMinimal.h"
#include "Access/HLWAccessDeviceBase.h"
#include "Access/HLWAccessTypes.h"
#include "HLWInteractableInterface.h"
#include "HLWKeycardReader.generated.h"

class UAudioComponent;
class UPointLightComponent;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHLWOnReaderResult, EHLWAccessResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHLWOnReaderStateChanged, EHLWReaderState, NewState);

// Card reader that asks the access subsystem and forwards the result to its controlled actor
UCLASS()
class THE_HOLLOW_API AHLWKeycardReader : public AHLWAccessDeviceBase, public IHLWInteractableInterface
{
	GENERATED_BODY()

public:
	AHLWKeycardReader();

	UFUNCTION(BlueprintCallable, Category = "HLW|Reader")
	void SetAccessDisabled(bool bDisabled);

	// True once a scan has succeeded; the light then rests on green
	UFUNCTION(BlueprintPure, Category = "HLW|Reader")
	bool IsUnlocked() const { return bUnlocked; }

	UFUNCTION(BlueprintPure, Category = "HLW|Reader")
	EHLWReaderState GetReaderState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "HLW|Reader")
	const FHLWFacilityAccessData& GetAccessData() const { return AccessData; }

	UPROPERTY(BlueprintAssignable, Category = "HLW|Reader")
	FHLWOnReaderResult OnAccessResult;

	UPROPERTY(BlueprintAssignable, Category = "HLW|Reader")
	FHLWOnReaderStateChanged OnStateChanged;

	virtual void Interact_Implementation(AActor* InteractingActor) override;
	virtual FText GetInteractionText_Implementation() override;
	virtual bool CanInteract_Implementation() override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnDeviceStateChanged(bool bPowerChanged) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Reader")
	TObjectPtr<UPointLightComponent> StatusLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Reader")
	TObjectPtr<UAudioComponent> Audio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Reader")
	FHLWFacilityAccessData AccessData;

	// Must implement HLWAccessControllable
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "HLW|Reader")
	TObjectPtr<AActor> ControlledActor;

	UPROPERTY(EditAnywhere, Category = "HLW|Reader|Timing", meta = (ClampMin = "0"))
	float ScanDuration = 0.75f;

	// How long a red or amber result holds before the light rests again
	UPROPERTY(EditAnywhere, Category = "HLW|Reader|Timing", meta = (ClampMin = "0.1"))
	float ResultDisplayTime = 2.f;

	UPROPERTY(EditAnywhere, Category = "HLW|Reader|Light")
	float LightIntensity = 800.f;

	// Resting light, marks the panel
	UPROPERTY(EditAnywhere, Category = "HLW|Reader|Light")
	FLinearColor IdleColor = FLinearColor(0.1f, 0.35f, 1.f);

	UPROPERTY(EditAnywhere, Category = "HLW|Reader|Light")
	FLinearColor ScanningColor = FLinearColor(0.6f, 0.85f, 1.f);

	// Success, stays on once unlocked
	UPROPERTY(EditAnywhere, Category = "HLW|Reader|Light")
	FLinearColor GrantedColor = FLinearColor::Green;

	// Locked, card missing or not accepted
	UPROPERTY(EditAnywhere, Category = "HLW|Reader|Light")
	FLinearColor DeniedColor = FLinearColor::Red;

	// Locked by lockdown or disabled access
	UPROPERTY(EditAnywhere, Category = "HLW|Reader|Light")
	FLinearColor LockedColor = FLinearColor(1.f, 0.45f, 0.f);

	UPROPERTY(EditAnywhere, Category = "HLW|Reader|Audio")
	TObjectPtr<USoundBase> ScanSound;

	UPROPERTY(EditAnywhere, Category = "HLW|Reader|Audio")
	TObjectPtr<USoundBase> GrantedSound;

	UPROPERTY(EditAnywhere, Category = "HLW|Reader|Audio")
	TObjectPtr<USoundBase> DeniedSound;

	UPROPERTY(EditAnywhere, Category = "HLW|Reader|Audio")
	TObjectPtr<USoundBase> OfflineSound;

	// Prints each result as a debug message, on top of the normal UI
	UPROPERTY(EditAnywhere, Category = "HLW|Reader")
	bool bShowDebugResult = false;

private:
	UFUNCTION()
	void HandleLockdownChanged(FName Zone, bool bActive);

	UFUNCTION()
	void HandleStateRestored();

	void ResolveAccess();
	void ReturnToIdle();
	void RefreshRestingState();
	EHLWReaderState GetRestingState() const;
	bool IsLockedBySystem() const;
	void SetState(EHLWReaderState NewState);
	void UpdateLight();
	void PlaySound(USoundBase* Sound);
	void NotifyPower();
	void NotifySecurityLock();
	bool HasControllable() const;

	// Power and network only count when the access data requires them
	bool IsAvailable() const
	{
		return (!AccessData.bRequiresPower || bPowered)
			&& (!AccessData.bRequiresNetwork || bNetworked)
			&& bOperational;
	}

	EHLWReaderState State = EHLWReaderState::Idle;
	bool bUnlocked = false;
	TWeakObjectPtr<AActor> PendingUser;
	FTimerHandle ScanTimer;
	FTimerHandle ResultTimer;
};
