#pragma once

#include "CoreMinimal.h"
#include "HLWAccessTypes.generated.h"

class AActor;
class UHLWKeycardDefinition;

DECLARE_LOG_CATEGORY_EXTERN(LogHLWAccess, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHLWOnKeycardCollected, UHLWKeycardDefinition*, Keycard, AActor*, Collector);

// Higher levels do not imply lower ones; devices list what they accept
UENUM(BlueprintType)
enum class EHLWKeycardAccessLevel : uint8
{
	None,
	General,
	Maintenance,
	Research,
	Security,
	Restricted
};

UENUM(BlueprintType)
enum class EHLWAccessResult : uint8
{
	Granted,
	MissingKeycard          UMETA(DisplayName = "Missing Keycard"),
	InsufficientAccessLevel UMETA(DisplayName = "Insufficient Access Level"),
	InvalidCredential       UMETA(DisplayName = "Invalid Credential"),
	DeviceOffline           UMETA(DisplayName = "Device Offline"),
	NoPower                 UMETA(DisplayName = "No Power"),
	NetworkOffline          UMETA(DisplayName = "Network Offline"),
	SecurityLockdown        UMETA(DisplayName = "Security Lockdown"),
	AccessDisabled          UMETA(DisplayName = "Access Disabled"),
	InvalidDevice           UMETA(DisplayName = "Invalid Device")
};

UENUM(BlueprintType)
enum class EHLWAccessDeviceType : uint8
{
	Door,
	Elevator,
	Terminal,
	Equipment
};

// What a controlled device does when it loses power
UENUM(BlueprintType)
enum class EHLWPowerFailureBehavior : uint8
{
	StayLocked UMETA(ToolTip = "Can't move while unpowered and keeps its lock"),
	Unlock     UMETA(ToolTip = "Lock releases while unpowered, can be opened by hand"),
	KeepState  UMETA(ToolTip = "Power has no effect on the device")
};

// Reader light: Idle blue, Scanning pale blue, Granted green, Denied red, Locked amber, Offline off
UENUM(BlueprintType)
enum class EHLWReaderState : uint8
{
	Idle,
	Scanning,
	Granted,
	Denied,
	Locked,
	Offline
};

// Cards that open a device: any listed level or any listed card ID
USTRUCT(BlueprintType)
struct FHLWAccessRequirement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Access")
	TArray<EHLWKeycardAccessLevel> AcceptedLevels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Access")
	TArray<FName> AcceptedKeycardIDs;

	bool IsEmpty() const { return AcceptedLevels.IsEmpty() && AcceptedKeycardIDs.IsEmpty(); }
};

// Per-device access configuration
USTRUCT(BlueprintType)
struct FHLWFacilityAccessData
{
	GENERATED_BODY()

	// Stable ID for saves and events. Readers use their actor name when empty.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Access")
	FName DeviceID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Access")
	EHLWAccessDeviceType DeviceType = EHLWAccessDeviceType::Door;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Access")
	FHLWAccessRequirement Requirement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Access")
	bool bRequiresPower = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Access")
	bool bRequiresNetwork = false;

	// Devices in a locked-down zone reject all cards
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Access")
	FName LockdownZone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Access")
	bool bAccessDisabled = false;
};

namespace HLWAccess
{
	// "Level.ActorName", stable across play sessions and unique across levels
	THE_HOLLOW_API FName MakeStableID(const AActor* Actor);

	// Results caused by the device rather than the player's cards
	inline bool IsOfflineResult(EHLWAccessResult Result)
	{
		return Result == EHLWAccessResult::NoPower
			|| Result == EHLWAccessResult::NetworkOffline
			|| Result == EHLWAccessResult::DeviceOffline;
	}

	// Results caused by the player's cards
	inline bool IsCredentialResult(EHLWAccessResult Result)
	{
		return Result == EHLWAccessResult::MissingKeycard
			|| Result == EHLWAccessResult::InsufficientAccessLevel
			|| Result == EHLWAccessResult::InvalidCredential;
	}
}

// Live state of a device at the moment of a request
struct FHLWAccessDeviceStatus
{
	bool bPowered = true;
	bool bNetworked = true;
	bool bOperational = true;
};
