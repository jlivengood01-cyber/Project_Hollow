#include "Access/HLWAccessStateSubsystem.h"

#include "Access/HLWAccessTypes.h"
#include "Access/HLWKeycardDefinition.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UHLWAccessStateSubsystem* UHLWAccessStateSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UHLWAccessStateSubsystem>() : nullptr;
}

bool UHLWAccessStateSubsystem::SaveToSlot(const FString& SlotName, int32 UserIndex)
{
	UHLWAccessSaveGame* SaveGame = Cast<UHLWAccessSaveGame>(UGameplayStatics::CreateSaveGameObject(UHLWAccessSaveGame::StaticClass()));
	if (!SaveGame)
	{
		return false;
	}

	SaveGame->Data = Data;
	const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, UserIndex);
	UE_LOG(LogHLWAccess, Log, TEXT("Access save to %s: %s"), *SlotName, bSaved ? TEXT("ok") : TEXT("failed"));
	return bSaved;
}

bool UHLWAccessStateSubsystem::LoadFromSlot(const FString& SlotName, int32 UserIndex)
{
	const UHLWAccessSaveGame* SaveGame = Cast<UHLWAccessSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
	if (!SaveGame)
	{
		UE_LOG(LogHLWAccess, Warning, TEXT("Access load from %s: no save found"), *SlotName);
		return false;
	}

	ApplySaveData(SaveGame->Data);
	UE_LOG(LogHLWAccess, Log, TEXT("Access load from %s: ok"), *SlotName);
	return true;
}

void UHLWAccessStateSubsystem::ResetState()
{
	ApplySaveData(FHLWAccessSaveData());
}

void UHLWAccessStateSubsystem::ApplySaveData(const FHLWAccessSaveData& NewData)
{
	Data = NewData;
	OnStateRestored.Broadcast();
}

void UHLWAccessStateSubsystem::SetKeycards(const TArray<UHLWKeycardDefinition*>& Keycards)
{
	Data.Keycards.Reset(Keycards.Num());
	for (UHLWKeycardDefinition* Card : Keycards)
	{
		if (Card)
		{
			Data.Keycards.Add(Card);
		}
	}
}

TArray<UHLWKeycardDefinition*> UHLWAccessStateSubsystem::LoadKeycards() const
{
	TArray<UHLWKeycardDefinition*> Result;
	for (const TSoftObjectPtr<UHLWKeycardDefinition>& Card : Data.Keycards)
	{
		if (UHLWKeycardDefinition* Loaded = Card.LoadSynchronous())
		{
			Result.Add(Loaded);
		}
	}
	return Result;
}

void UHLWAccessStateSubsystem::SetPickupCollected(FName PickupID)
{
	Data.CollectedPickups.Add(PickupID);
}

void UHLWAccessStateSubsystem::SetDeviceUnlocked(FName DeviceID, bool bUnlocked)
{
	if (bUnlocked)
	{
		Data.UnlockedDevices.Add(DeviceID);
	}
	else
	{
		Data.UnlockedDevices.Remove(DeviceID);
	}
}

void UHLWAccessStateSubsystem::SetZoneLocked(FName Zone, bool bLocked)
{
	if (bLocked)
	{
		Data.LockedZones.Add(Zone);
	}
	else
	{
		Data.LockedZones.Remove(Zone);
	}
}

void UHLWAccessStateSubsystem::SetKeycardRevoked(FName KeycardID, bool bRevoked)
{
	if (bRevoked)
	{
		Data.RevokedKeycards.Add(KeycardID);
	}
	else
	{
		Data.RevokedKeycards.Remove(KeycardID);
	}
}

void UHLWAccessStateSubsystem::SetObjectiveStage(FName ObjectiveID, uint8 Stage)
{
	Data.ObjectiveStages.Add(ObjectiveID, Stage);
}

uint8 UHLWAccessStateSubsystem::GetObjectiveStage(FName ObjectiveID) const
{
	const uint8* Found = Data.ObjectiveStages.Find(ObjectiveID);
	return Found ? *Found : 0;
}
