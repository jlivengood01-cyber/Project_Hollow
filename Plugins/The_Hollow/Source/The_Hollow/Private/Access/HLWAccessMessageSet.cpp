#include "Access/HLWAccessMessageSet.h"

#define LOCTEXT_NAMESPACE "HLWAccessMessages"

FText UHLWAccessMessageSet::GetMessage(EHLWAccessResult Result) const
{
	const FText* Found = Messages.Find(Result);
	return Found && !Found->IsEmpty() ? *Found : GetDefaultMessage(Result);
}

FText UHLWAccessMessageSet::GetDefaultMessage(EHLWAccessResult Result)
{
	switch (Result)
	{
	case EHLWAccessResult::Granted:                 return LOCTEXT("Granted", "Access granted.");
	case EHLWAccessResult::MissingKeycard:          return LOCTEXT("MissingKeycard", "A keycard is required.");
	case EHLWAccessResult::InsufficientAccessLevel: return LOCTEXT("Insufficient", "Insufficient clearance.");
	case EHLWAccessResult::InvalidCredential:       return LOCTEXT("Invalid", "Credential rejected.");
	case EHLWAccessResult::DeviceOffline:           return LOCTEXT("DeviceOffline", "The reader is unresponsive.");
	case EHLWAccessResult::NoPower:                 return LOCTEXT("NoPower", "No power to the reader.");
	case EHLWAccessResult::NetworkOffline:          return LOCTEXT("NetworkOffline", "Security network offline.");
	case EHLWAccessResult::SecurityLockdown:        return LOCTEXT("Lockdown", "Lockdown in effect.");
	case EHLWAccessResult::AccessDisabled:          return LOCTEXT("Disabled", "Access has been disabled.");
	case EHLWAccessResult::InvalidDevice:           return LOCTEXT("InvalidDevice", "Reader fault.");
	default:                                        return FText::GetEmpty();
	}
}

#undef LOCTEXT_NAMESPACE
