#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HLWItemHolderComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHLWOnItemCountChanged, FName, ItemID, int32, Count);

// Stand-in item store for fuses and repair parts until SYS_PlayerInventory exists.
// Saved with the power state.
UCLASS(ClassGroup = (HLW), meta = (BlueprintSpawnableComponent))
class THE_HOLLOW_API UHLWItemHolderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHLWItemHolderComponent();

	UFUNCTION(BlueprintCallable, Category = "HLW|Items")
	void AddItem(FName ItemID, int32 Count = 1);

	// False if not enough are held
	UFUNCTION(BlueprintCallable, Category = "HLW|Items")
	bool RemoveItem(FName ItemID, int32 Count = 1);

	UFUNCTION(BlueprintPure, Category = "HLW|Items")
	bool HasItem(FName ItemID, int32 Count = 1) const { return GetItemCount(ItemID) >= Count; }

	UFUNCTION(BlueprintPure, Category = "HLW|Items")
	int32 GetItemCount(FName ItemID) const { return Items.FindRef(ItemID); }

	UPROPERTY(BlueprintAssignable, Category = "HLW|Items")
	FHLWOnItemCountChanged OnItemCountChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleStateRestored();

	void SaveState() const;

	UPROPERTY(VisibleInstanceOnly, Category = "HLW|Items")
	TMap<FName, int32> Items;
};
