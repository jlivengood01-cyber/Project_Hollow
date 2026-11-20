#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Access/HLWAccessControllable.h"
#include "HLWInteractableInterface.h"
#include "HLWAccessControlledDoor.generated.h"

class USoundBase;
class UStaticMeshComponent;

// Placeholder sliding door until SYS_DOOR_INTERACTION_001 exists
UCLASS()
class THE_HOLLOW_API AHLWAccessControlledDoor : public AActor, public IHLWAccessControllable, public IHLWInteractableInterface
{
	GENERATED_BODY()

public:
	AHLWAccessControlledDoor();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "HLW|Door")
	void SetDoorOpen(bool bOpen);

	// For the Electrical System; readers also forward their power here
	UFUNCTION(BlueprintCallable, Category = "HLW|Door")
	void SetPowered(bool bNewPowered);

	UFUNCTION(BlueprintCallable, Category = "HLW|Door")
	void SetSecurityLocked(bool bLocked);

	UFUNCTION(BlueprintPure, Category = "HLW|Door")
	bool IsDoorOpen() const { return bIsOpen; }

	UFUNCTION(BlueprintPure, Category = "HLW|Door")
	bool IsPowered() const { return bPowered; }

	UFUNCTION(BlueprintPure, Category = "HLW|Door")
	bool IsUnlocked() const;

	UFUNCTION(BlueprintPure, Category = "HLW|Door")
	bool CanMove() const;

	virtual void OnAccessGranted_Implementation(AActor* Reader) override;
	virtual void OnAccessDenied_Implementation(AActor* Reader, EHLWAccessResult Result) override;
	virtual void OnReaderPowerChanged_Implementation(AActor* Reader, bool bReaderPowered) override;
	virtual void OnSecurityLockChanged_Implementation(AActor* Reader, bool bLocked) override;
	virtual void OnAccessStateRestored_Implementation(AActor* Reader, bool bRestoredUnlocked) override;

	virtual void Interact_Implementation(AActor* InteractingActor) override;
	virtual FText GetInteractionText_Implementation() override;
	virtual bool CanInteract_Implementation() override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Door")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	// Off means a normal door that opens with interact
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Door")
	bool bRequiresAccess = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Door")
	bool bOpenOnGrant = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Door|Power")
	EHLWPowerFailureBehavior PowerFailureBehavior = EHLWPowerFailureBehavior::StayLocked;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Door|Security")
	bool bCloseOnLockdown = true;

	// Slide distance in local space
	UPROPERTY(EditAnywhere, Category = "HLW|Door")
	FVector OpenOffset = FVector(0.f, 120.f, 0.f);

	UPROPERTY(EditAnywhere, Category = "HLW|Door", meta = (ClampMin = "0.05"))
	float OpenTime = 1.f;

	UPROPERTY(EditAnywhere, Category = "HLW|Door|Audio")
	TObjectPtr<USoundBase> UnlockSound;

	UPROPERTY(EditAnywhere, Category = "HLW|Door|Audio")
	TObjectPtr<USoundBase> OpenSound;

	UPROPERTY(EditAnywhere, Category = "HLW|Door|Audio")
	TObjectPtr<USoundBase> CloseSound;

	UPROPERTY(EditAnywhere, Category = "HLW|Door|Audio")
	TObjectPtr<USoundBase> PowerLostSound;

private:
	void PlaySound(USoundBase* Sound) const;
	void UpdateTick();

	FVector ClosedLocation = FVector::ZeroVector;
	float OpenAlpha = 0.f;
	bool bIsOpen = false;
	bool bUnlocked = false;
	bool bPowered = true;
	bool bSecurityLocked = false;
};
