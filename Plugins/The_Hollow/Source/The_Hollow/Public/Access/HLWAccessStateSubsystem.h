#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HLWAccessStateSubsystem.generated.h"

class UHLWKeycardDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHLWOnAccessStateRestored);

// Everything the keycard system persists, by stable ID only
USTRUCT(BlueprintType)
struct FHLWAccessSaveData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HLW|Save")
	TArray<TSoftObjectPtr<UHLWKeycardDefinition>> Keycards;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HLW|Save")
	TSet<FName> CollectedPickups;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HLW|Save")
	TSet<FName> UnlockedDevices;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HLW|Save")
	TSet<FName> LockedZones;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HLW|Save")
	TSet<FName> RevokedKeycards;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HLW|Save")
	TMap<FName, uint8> ObjectiveStages;
};

UCLASS()
class THE_HOLLOW_API UHLWAccessSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	FHLWAccessSaveData Data;
};

// Holds access state across level changes and writes it to disk
UCLASS()
class THE_HOLLOW_API UHLWAccessStateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// Null outside a game world, such as in the editor
	static UHLWAccessStateSubsystem* Get(const UObject* WorldContext);

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
	const FHLWAccessSaveData& GetSaveData() const { return Data; }

	UFUNCTION(BlueprintCallable, Category = "HLW|Save")
	void ApplySaveData(const FHLWAccessSaveData& NewData);

	UPROPERTY(BlueprintAssignable, Category = "HLW|Save")
	FHLWOnAccessStateRestored OnStateRestored;

	void SetKeycards(const TArray<UHLWKeycardDefinition*>& Keycards);
	TArray<UHLWKeycardDefinition*> LoadKeycards() const;

	void SetPickupCollected(FName PickupID);
	bool IsPickupCollected(FName PickupID) const { return Data.CollectedPickups.Contains(PickupID); }

	void SetDeviceUnlocked(FName DeviceID, bool bUnlocked);
	bool IsDeviceUnlocked(FName DeviceID) const { return Data.UnlockedDevices.Contains(DeviceID); }

	void SetZoneLocked(FName Zone, bool bLocked);
	const TSet<FName>& GetLockedZones() const { return Data.LockedZones; }

	void SetKeycardRevoked(FName KeycardID, bool bRevoked);
	const TSet<FName>& GetRevokedKeycards() const { return Data.RevokedKeycards; }

	void SetObjectiveStage(FName ObjectiveID, uint8 Stage);
	uint8 GetObjectiveStage(FName ObjectiveID) const;

private:
	FHLWAccessSaveData Data;
};
