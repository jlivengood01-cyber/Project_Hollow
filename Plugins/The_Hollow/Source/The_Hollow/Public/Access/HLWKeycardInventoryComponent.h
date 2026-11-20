#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Access/HLWAccessTypes.h"
#include "HLWKeycardInventoryComponent.generated.h"

class UHLWKeycardDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHLWOnKeycardChanged, UHLWKeycardDefinition*, Keycard);

// Keycards the player holds, stored as data assets rather than pickup actors
UCLASS(ClassGroup = (HLW), meta = (BlueprintSpawnableComponent))
class THE_HOLLOW_API UHLWKeycardInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHLWKeycardInventoryComponent();

	// False if the card is invalid or a unique card is already held
	UFUNCTION(BlueprintCallable, Category = "HLW|Keycard")
	bool AddKeycard(UHLWKeycardDefinition* Keycard);

	UFUNCTION(BlueprintCallable, Category = "HLW|Keycard")
	bool RemoveKeycard(FName KeycardID);

	UFUNCTION(BlueprintPure, Category = "HLW|Keycard")
	bool HasKeycard(FName KeycardID) const;

	UFUNCTION(BlueprintPure, Category = "HLW|Keycard")
	bool HasAccessLevel(EHLWKeycardAccessLevel AccessLevel) const;

	UFUNCTION(BlueprintPure, Category = "HLW|Keycard")
	TArray<UHLWKeycardDefinition*> GetCollectedKeycards() const;

	UFUNCTION(BlueprintPure, Category = "HLW|Keycard")
	UHLWKeycardDefinition* GetKeycardData(FName KeycardID) const;

	UFUNCTION(BlueprintCallable, Category = "HLW|Keycard")
	void ClearKeycardInventory();

	UPROPERTY(BlueprintAssignable, Category = "HLW|Keycard")
	FHLWOnKeycardChanged OnKeycardAdded;

	UPROPERTY(BlueprintAssignable, Category = "HLW|Keycard")
	FHLWOnKeycardChanged OnKeycardRemoved;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Keeps cards through level changes and save/load. Use on the player only.
	UPROPERTY(EditAnywhere, Category = "HLW|Keycard")
	bool bPersistent = true;

private:
	UFUNCTION()
	void HandleStateRestored();

	void WriteToState() const;

	UPROPERTY(VisibleInstanceOnly, Category = "HLW|Keycard")
	TArray<TObjectPtr<UHLWKeycardDefinition>> Keycards;
};
