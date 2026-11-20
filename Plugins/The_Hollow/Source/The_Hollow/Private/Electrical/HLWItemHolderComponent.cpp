#include "Electrical/HLWItemHolderComponent.h"

#include "Electrical/HLWPowerStateSubsystem.h"

UHLWItemHolderComponent::UHLWItemHolderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHLWItemHolderComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UHLWPowerStateSubsystem* State = UHLWPowerStateSubsystem::Get(this))
	{
		State->OnStateRestored.AddDynamic(this, &ThisClass::HandleStateRestored);
		HandleStateRestored();
	}
}

void UHLWItemHolderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UHLWPowerStateSubsystem* State = UHLWPowerStateSubsystem::Get(this))
	{
		State->OnStateRestored.RemoveDynamic(this, &ThisClass::HandleStateRestored);
	}
	Super::EndPlay(EndPlayReason);
}

void UHLWItemHolderComponent::HandleStateRestored()
{
	if (const UHLWPowerStateSubsystem* State = UHLWPowerStateSubsystem::Get(this))
	{
		Items = State->GetHeldItems();
	}
}

void UHLWItemHolderComponent::SaveState() const
{
	if (UHLWPowerStateSubsystem* State = UHLWPowerStateSubsystem::Get(this))
	{
		State->SetHeldItems(Items);
	}
}

void UHLWItemHolderComponent::AddItem(FName ItemID, int32 Count)
{
	if (ItemID.IsNone() || Count <= 0)
	{
		return;
	}
	const int32 NewCount = (Items.FindOrAdd(ItemID) += Count);
	SaveState();
	OnItemCountChanged.Broadcast(ItemID, NewCount);
}

bool UHLWItemHolderComponent::RemoveItem(FName ItemID, int32 Count)
{
	int32* Held = Items.Find(ItemID);
	if (!Held || Count <= 0 || *Held < Count)
	{
		return false;
	}

	*Held -= Count;
	const int32 NewCount = *Held;
	if (NewCount == 0)
	{
		Items.Remove(ItemID);
	}
	SaveState();
	OnItemCountChanged.Broadcast(ItemID, NewCount);
	return true;
}
