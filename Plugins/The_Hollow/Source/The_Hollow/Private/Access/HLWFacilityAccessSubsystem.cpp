#include "Access/HLWFacilityAccessSubsystem.h"

#include "Access/HLWAccessDeviceState.h"
#include "Access/HLWAccessStateSubsystem.h"
#include "Access/HLWKeycardDefinition.h"
#include "Access/HLWKeycardInventoryComponent.h"

DEFINE_LOG_CATEGORY(LogHLWAccess);

void UHLWFacilityAccessSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (UHLWAccessStateSubsystem* State = UHLWAccessStateSubsystem::Get(this))
	{
		State->OnStateRestored.AddDynamic(this, &ThisClass::HandleStateRestored);
		LockedZones = State->GetLockedZones();
		RevokedKeycards = State->GetRevokedKeycards();
	}
}

void UHLWFacilityAccessSubsystem::Deinitialize()
{
	if (UHLWAccessStateSubsystem* State = UHLWAccessStateSubsystem::Get(this))
	{
		State->OnStateRestored.RemoveDynamic(this, &ThisClass::HandleStateRestored);
	}
	Super::Deinitialize();
}

void UHLWFacilityAccessSubsystem::HandleStateRestored()
{
	const UHLWAccessStateSubsystem* State = UHLWAccessStateSubsystem::Get(this);
	if (!State)
	{
		return;
	}

	// Go through SetLockdown so readers and doors hear about each change
	const TSet<FName> Saved = State->GetLockedZones();
	for (const FName& Zone : LockedZones.Difference(Saved))
	{
		SetLockdown(Zone, false);
	}
	for (const FName& Zone : Saved.Difference(LockedZones))
	{
		SetLockdown(Zone, true);
	}
	RevokedKeycards = State->GetRevokedKeycards();
}

EHLWAccessResult UHLWFacilityAccessSubsystem::RequestAccess(const UHLWKeycardInventoryComponent* Inventory, const FHLWFacilityAccessData& AccessData, UObject* Device)
{
	const EHLWAccessResult Result = CheckAccess(Inventory, AccessData, Device);
	ReportAccessResult(AccessData.DeviceID, Result);
	return Result;
}

void UHLWFacilityAccessSubsystem::ReportAccessResult(FName DeviceID, EHLWAccessResult Result)
{
	UE_LOG(LogHLWAccess, Log, TEXT("Access %s: %s"), *DeviceID.ToString(), *UEnum::GetDisplayValueAsText(Result).ToString());
	OnAccessResolved.Broadcast(DeviceID, Result);
}

EHLWAccessResult UHLWFacilityAccessSubsystem::CheckAccess(const UHLWKeycardInventoryComponent* Inventory, const FHLWFacilityAccessData& AccessData, UObject* Device) const
{
	const TArray<UHLWKeycardDefinition*> Keycards = Inventory ? Inventory->GetCollectedKeycards() : TArray<UHLWKeycardDefinition*>();
	return EvaluateAccess(Keycards, AccessData, GetDeviceStatus(Device));
}

EHLWAccessResult UHLWFacilityAccessSubsystem::EvaluateAccess(const TArray<UHLWKeycardDefinition*>& Keycards, const FHLWFacilityAccessData& AccessData, const FHLWAccessDeviceStatus& Status) const
{
	EHLWAccessResult Result = CheckDeviceAvailability(AccessData, Status);
	if (Result != EHLWAccessResult::Granted)
	{
		return Result;
	}

	Result = CheckSecurityRestrictions(AccessData);
	if (Result != EHLWAccessResult::Granted)
	{
		return Result;
	}

	return ValidateCredentials(Keycards, AccessData.Requirement);
}

EHLWAccessResult UHLWFacilityAccessSubsystem::CheckDeviceAvailability(const FHLWFacilityAccessData& AccessData, const FHLWAccessDeviceStatus& Status)
{
	// An empty requirement is treated as a setup mistake, not an open door
	if (AccessData.DeviceID.IsNone() || AccessData.Requirement.IsEmpty())
	{
		return EHLWAccessResult::InvalidDevice;
	}
	if (AccessData.bAccessDisabled)
	{
		return EHLWAccessResult::AccessDisabled;
	}
	if (AccessData.bRequiresPower && !Status.bPowered)
	{
		return EHLWAccessResult::NoPower;
	}
	if (AccessData.bRequiresNetwork && !Status.bNetworked)
	{
		return EHLWAccessResult::NetworkOffline;
	}
	if (!Status.bOperational)
	{
		return EHLWAccessResult::DeviceOffline;
	}
	return EHLWAccessResult::Granted;
}

EHLWAccessResult UHLWFacilityAccessSubsystem::CheckSecurityRestrictions(const FHLWFacilityAccessData& AccessData) const
{
	return IsZoneLockedDown(AccessData.LockdownZone) ? EHLWAccessResult::SecurityLockdown : EHLWAccessResult::Granted;
}

EHLWAccessResult UHLWFacilityAccessSubsystem::ValidateCredentials(const TArray<UHLWKeycardDefinition*>& Keycards, const FHLWAccessRequirement& Requirement) const
{
	bool bMatchedRevoked = false;

	for (const UHLWKeycardDefinition* Card : Keycards)
	{
		if (!Card)
		{
			continue;
		}

		const bool bMatches = Requirement.AcceptedKeycardIDs.Contains(Card->KeycardID)
			|| Requirement.AcceptedLevels.Contains(Card->AccessLevel);
		if (!bMatches)
		{
			continue;
		}
		if (!IsKeycardRevoked(Card->KeycardID))
		{
			return EHLWAccessResult::Granted;
		}
		bMatchedRevoked = true;
	}

	if (bMatchedRevoked)
	{
		return EHLWAccessResult::InvalidCredential;
	}

	// Level-based devices report the level, ID-only devices report the missing card
	const bool bHasAnyCard = Keycards.ContainsByPredicate([](const UHLWKeycardDefinition* Card) { return Card != nullptr; });
	return bHasAnyCard && !Requirement.AcceptedLevels.IsEmpty()
		? EHLWAccessResult::InsufficientAccessLevel
		: EHLWAccessResult::MissingKeycard;
}

FHLWAccessDeviceStatus UHLWFacilityAccessSubsystem::GetDeviceStatus(UObject* Device)
{
	// Devices without the interface count as powered, networked and working
	FHLWAccessDeviceStatus Status;
	if (Device && Device->Implements<UHLWAccessDeviceState>())
	{
		Status.bPowered = IHLWAccessDeviceState::Execute_IsPowered(Device);
		Status.bNetworked = IHLWAccessDeviceState::Execute_IsNetworked(Device);
		Status.bOperational = IHLWAccessDeviceState::Execute_IsOperational(Device);
	}
	return Status;
}

void UHLWFacilityAccessSubsystem::SetLockdown(FName Zone, bool bActive)
{
	if (Zone.IsNone() || IsZoneLockedDown(Zone) == bActive)
	{
		return;
	}
	if (bActive)
	{
		LockedZones.Add(Zone);
	}
	else
	{
		LockedZones.Remove(Zone);
	}
	if (UHLWAccessStateSubsystem* State = UHLWAccessStateSubsystem::Get(this))
	{
		State->SetZoneLocked(Zone, bActive);
	}
	OnLockdownChanged.Broadcast(Zone, bActive);
}

bool UHLWFacilityAccessSubsystem::IsZoneLockedDown(FName Zone) const
{
	return !Zone.IsNone() && LockedZones.Contains(Zone);
}

void UHLWFacilityAccessSubsystem::SetKeycardRevoked(FName KeycardID, bool bRevoked)
{
	if (bRevoked)
	{
		RevokedKeycards.Add(KeycardID);
	}
	else
	{
		RevokedKeycards.Remove(KeycardID);
	}
	if (UHLWAccessStateSubsystem* State = UHLWAccessStateSubsystem::Get(this))
	{
		State->SetKeycardRevoked(KeycardID, bRevoked);
	}
}

bool UHLWFacilityAccessSubsystem::IsKeycardRevoked(FName KeycardID) const
{
	return RevokedKeycards.Contains(KeycardID);
}
