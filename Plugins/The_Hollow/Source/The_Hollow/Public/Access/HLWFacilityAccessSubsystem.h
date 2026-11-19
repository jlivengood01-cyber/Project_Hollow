#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Access/HLWAccessTypes.h"
#include "HLWFacilityAccessSubsystem.generated.h"

class UHLWKeycardDefinition;
class UHLWKeycardInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHLWOnAccessResolved, FName, DeviceID, EHLWAccessResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHLWOnLockdownChanged, FName, Zone, bool, bActive);

// Single authority for access rules. Returns a result, never operates devices.
UCLASS()
class THE_HOLLOW_API UHLWFacilityAccessSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// Checks access and broadcasts OnAccessResolved
	UFUNCTION(BlueprintCallable, Category = "HLW|Access")
	EHLWAccessResult RequestAccess(const UHLWKeycardInventoryComponent* Inventory, const FHLWFacilityAccessData& AccessData, UObject* Device);

	// Same check without broadcasting, for previews like elevator floor lists
	UFUNCTION(BlueprintPure, Category = "HLW|Access")
	EHLWAccessResult CheckAccess(const UHLWKeycardInventoryComponent* Inventory, const FHLWFacilityAccessData& AccessData, UObject* Device) const;

	UFUNCTION(BlueprintCallable, Category = "HLW|Access")
	void SetLockdown(FName Zone, bool bActive);

	UFUNCTION(BlueprintPure, Category = "HLW|Access")
	bool IsZoneLockedDown(FName Zone) const;

	// Revoked cards still match but are rejected as invalid
	UFUNCTION(BlueprintCallable, Category = "HLW|Access")
	void SetKeycardRevoked(FName KeycardID, bool bRevoked);

	UFUNCTION(BlueprintPure, Category = "HLW|Access")
	bool IsKeycardRevoked(FName KeycardID) const;

	UPROPERTY(BlueprintAssignable, Category = "HLW|Access")
	FHLWOnAccessResolved OnAccessResolved;

	UPROPERTY(BlueprintAssignable, Category = "HLW|Access")
	FHLWOnLockdownChanged OnLockdownChanged;

	// Any keycard picked up in this world, for objectives
	UPROPERTY(BlueprintAssignable, Category = "HLW|Access")
	FHLWOnKeycardCollected OnKeycardCollected;

	// Broadcasts a result a device decided itself, such as an unpowered elevator
	UFUNCTION(BlueprintCallable, Category = "HLW|Access")
	void ReportAccessResult(FName DeviceID, EHLWAccessResult Result);

	// Device checks first, then security, then credentials
	EHLWAccessResult EvaluateAccess(const TArray<UHLWKeycardDefinition*>& Keycards, const FHLWFacilityAccessData& AccessData, const FHLWAccessDeviceStatus& Status) const;

	static EHLWAccessResult CheckDeviceAvailability(const FHLWFacilityAccessData& AccessData, const FHLWAccessDeviceStatus& Status);
	EHLWAccessResult CheckSecurityRestrictions(const FHLWFacilityAccessData& AccessData) const;
	EHLWAccessResult ValidateCredentials(const TArray<UHLWKeycardDefinition*>& Keycards, const FHLWAccessRequirement& Requirement) const;

	static FHLWAccessDeviceStatus GetDeviceStatus(UObject* Device);

private:
	UFUNCTION()
	void HandleStateRestored();

	TSet<FName> LockedZones;
	TSet<FName> RevokedKeycards;
};
