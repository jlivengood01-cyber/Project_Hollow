#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Electrical/HLWPowerTypes.h"
#include "HLWPowerStateSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHLWOnPowerStateRestored);

USTRUCT(BlueprintType)
struct FHLWGeneratorSaveData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HLW|Save")
	EHLWGeneratorState State = EHLWGeneratorState::Off;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HLW|Save")
	float Fuel = 0.f;
};

// Breakers and fuseboxes
USTRUCT(BlueprintType)
struct FHLWSwitchSaveData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HLW|Save")
	bool bOn = false;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HLW|Save")
	FName InstalledFuse;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HLW|Save")
	bool bInspected = false;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HLW|Save")
	bool bFaulted = false;
};

// Everything the electrical system persists, by stable ID only. Missing entries mean editor defaults.
USTRUCT(BlueprintType)
struct FHLWPowerSaveData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HLW|Save")
	TMap<FName, FHLWGeneratorSaveData> Generators;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HLW|Save")
	TMap<FName, bool> CircuitEnabled;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HLW|Save")
	TMap<FName, FHLWSwitchSaveData> Switches;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HLW|Save")
	TMap<FName, bool> Faults;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HLW|Save")
	TSet<FName> CollectedPickups;

	// Stand-in item holder contents
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HLW|Save")
	TMap<FName, int32> HeldItems;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HLW|Save")
	TMap<FName, uint8> ObjectiveStages;
};

UCLASS()
class THE_HOLLOW_API UHLWPowerSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	FHLWPowerSaveData Data;
};

// Holds power state across level changes and writes it to disk
UCLASS()
class THE_HOLLOW_API UHLWPowerStateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// Null outside a game world
	static UHLWPowerStateSubsystem* Get(const UObject* WorldContext);

	UFUNCTION(BlueprintCallable, Category = "HLW|Save")
	bool SaveToSlot(const FString& SlotName, int32 UserIndex = 0);

	// Live actors resync through OnStateRestored
	UFUNCTION(BlueprintCallable, Category = "HLW|Save")
	bool LoadFromSlot(const FString& SlotName, int32 UserIndex = 0);

	// Clears everything, for a new game
	UFUNCTION(BlueprintCallable, Category = "HLW|Save")
	void ResetState();

	// For the project-wide Save System to embed
	UFUNCTION(BlueprintPure, Category = "HLW|Save")
	const FHLWPowerSaveData& GetSaveData() const { return Data; }

	UFUNCTION(BlueprintCallable, Category = "HLW|Save")
	void ApplySaveData(const FHLWPowerSaveData& NewData);

	UPROPERTY(BlueprintAssignable, Category = "HLW|Save")
	FHLWOnPowerStateRestored OnStateRestored;

	const FHLWGeneratorSaveData* FindGenerator(FName ID) const { return Data.Generators.Find(ID); }
	void SetGenerator(FName ID, EHLWGeneratorState State, float Fuel);

	const TMap<FName, bool>& GetCircuitEnabled() const { return Data.CircuitEnabled; }
	void SetCircuitEnabled(FName CircuitID, bool bEnabled) { Data.CircuitEnabled.Add(CircuitID, bEnabled); }

	const FHLWSwitchSaveData* FindSwitch(FName ID) const { return Data.Switches.Find(ID); }
	void SetSwitch(FName ID, const FHLWSwitchSaveData& SwitchData) { Data.Switches.Add(ID, SwitchData); }

	const bool* FindFault(FName FaultID) const { return Data.Faults.Find(FaultID); }
	void SetFault(FName FaultID, bool bActive) { Data.Faults.Add(FaultID, bActive); }

	bool IsPickupCollected(FName PickupID) const { return Data.CollectedPickups.Contains(PickupID); }
	void SetPickupCollected(FName PickupID) { Data.CollectedPickups.Add(PickupID); }

	const TMap<FName, int32>& GetHeldItems() const { return Data.HeldItems; }
	void SetHeldItems(const TMap<FName, int32>& Items) { Data.HeldItems = Items; }

	const uint8* FindObjectiveStage(FName ObjectiveID) const { return Data.ObjectiveStages.Find(ObjectiveID); }
	void SetObjectiveStage(FName ObjectiveID, uint8 Stage) { Data.ObjectiveStages.Add(ObjectiveID, Stage); }

private:
	FHLWPowerSaveData Data;
};
