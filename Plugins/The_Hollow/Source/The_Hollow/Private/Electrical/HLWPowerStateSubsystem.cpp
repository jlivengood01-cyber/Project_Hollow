#include "Electrical/HLWPowerStateSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UHLWPowerStateSubsystem* UHLWPowerStateSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UHLWPowerStateSubsystem>() : nullptr;
}

bool UHLWPowerStateSubsystem::SaveToSlot(const FString& SlotName, int32 UserIndex)
{
	UHLWPowerSaveGame* SaveGame = Cast<UHLWPowerSaveGame>(UGameplayStatics::CreateSaveGameObject(UHLWPowerSaveGame::StaticClass()));
	if (!SaveGame)
	{
		return false;
	}

	SaveGame->Data = Data;
	const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, UserIndex);
	UE_LOG(LogHLWPower, Log, TEXT("Power save to %s: %s"), *SlotName, bSaved ? TEXT("ok") : TEXT("failed"));
	return bSaved;
}

bool UHLWPowerStateSubsystem::LoadFromSlot(const FString& SlotName, int32 UserIndex)
{
	const UHLWPowerSaveGame* SaveGame = Cast<UHLWPowerSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
	if (!SaveGame)
	{
		UE_LOG(LogHLWPower, Warning, TEXT("Power load from %s: no save found"), *SlotName);
		return false;
	}

	ApplySaveData(SaveGame->Data);
	UE_LOG(LogHLWPower, Log, TEXT("Power load from %s: ok"), *SlotName);
	return true;
}

void UHLWPowerStateSubsystem::ResetState()
{
	ApplySaveData(FHLWPowerSaveData());
}

void UHLWPowerStateSubsystem::ApplySaveData(const FHLWPowerSaveData& NewData)
{
	Data = NewData;
	OnStateRestored.Broadcast();
}

void UHLWPowerStateSubsystem::SetGenerator(FName ID, EHLWGeneratorState State, float Fuel)
{
	FHLWGeneratorSaveData& Entry = Data.Generators.FindOrAdd(ID);
	Entry.State = State;
	Entry.Fuel = Fuel;
}
