#include "Access/HLWKeycardInventoryComponent.h"

#include "Access/HLWAccessStateSubsystem.h"
#include "Access/HLWKeycardDefinition.h"

UHLWKeycardInventoryComponent::UHLWKeycardInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHLWKeycardInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UHLWAccessStateSubsystem* State = bPersistent ? UHLWAccessStateSubsystem::Get(this) : nullptr)
	{
		State->OnStateRestored.AddDynamic(this, &ThisClass::HandleStateRestored);
		HandleStateRestored();
	}
}

void UHLWKeycardInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UHLWAccessStateSubsystem* State = UHLWAccessStateSubsystem::Get(this))
	{
		State->OnStateRestored.RemoveDynamic(this, &ThisClass::HandleStateRestored);
	}
	Super::EndPlay(EndPlayReason);
}

void UHLWKeycardInventoryComponent::HandleStateRestored()
{
	// Silent restore, no pickup notifications
	if (const UHLWAccessStateSubsystem* State = UHLWAccessStateSubsystem::Get(this))
	{
		Keycards = TArray<TObjectPtr<UHLWKeycardDefinition>>(State->LoadKeycards());
	}
}

void UHLWKeycardInventoryComponent::WriteToState() const
{
	if (UHLWAccessStateSubsystem* State = bPersistent ? UHLWAccessStateSubsystem::Get(this) : nullptr)
	{
		State->SetKeycards(GetCollectedKeycards());
	}
}

bool UHLWKeycardInventoryComponent::AddKeycard(UHLWKeycardDefinition* Keycard)
{
	if (!Keycard || Keycard->KeycardID.IsNone())
	{
		UE_LOG(LogHLWAccess, Warning, TEXT("AddKeycard: missing keycard or KeycardID"));
		return false;
	}
	if (Keycard->bUnique && HasKeycard(Keycard->KeycardID))
	{
		UE_LOG(LogHLWAccess, Log, TEXT("AddKeycard: %s already held"), *Keycard->KeycardID.ToString());
		return false;
	}

	Keycards.Add(Keycard);
	WriteToState();
	OnKeycardAdded.Broadcast(Keycard);
	return true;
}

bool UHLWKeycardInventoryComponent::RemoveKeycard(FName KeycardID)
{
	const int32 Index = Keycards.IndexOfByPredicate([KeycardID](const UHLWKeycardDefinition* Card)
	{
		return Card && Card->KeycardID == KeycardID;
	});
	if (Index == INDEX_NONE)
	{
		return false;
	}

	UHLWKeycardDefinition* Removed = Keycards[Index];
	Keycards.RemoveAt(Index);
	WriteToState();
	OnKeycardRemoved.Broadcast(Removed);
	return true;
}

bool UHLWKeycardInventoryComponent::HasKeycard(FName KeycardID) const
{
	return GetKeycardData(KeycardID) != nullptr;
}

bool UHLWKeycardInventoryComponent::HasAccessLevel(EHLWKeycardAccessLevel AccessLevel) const
{
	return Keycards.ContainsByPredicate([AccessLevel](const UHLWKeycardDefinition* Card)
	{
		return Card && Card->AccessLevel == AccessLevel;
	});
}

TArray<UHLWKeycardDefinition*> UHLWKeycardInventoryComponent::GetCollectedKeycards() const
{
	return TArray<UHLWKeycardDefinition*>(Keycards);
}

UHLWKeycardDefinition* UHLWKeycardInventoryComponent::GetKeycardData(FName KeycardID) const
{
	const TObjectPtr<UHLWKeycardDefinition>* Found = Keycards.FindByPredicate([KeycardID](const UHLWKeycardDefinition* Card)
	{
		return Card && Card->KeycardID == KeycardID;
	});
	return Found ? Found->Get() : nullptr;
}

void UHLWKeycardInventoryComponent::ClearKeycardInventory()
{
	TArray<TObjectPtr<UHLWKeycardDefinition>> Removed = MoveTemp(Keycards);
	Keycards.Reset();
	WriteToState();
	for (UHLWKeycardDefinition* Card : Removed)
	{
		OnKeycardRemoved.Broadcast(Card);
	}
}
