#include "Characters/Test/HLWAccessTestSwitch.h"

#include "Access/HLWAccessDeviceBase.h"
#include "Access/HLWAccessDeviceState.h"
#include "Access/HLWFacilityAccessSubsystem.h"
#include "Access/HLWKeycardReader.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "HLWAccessTestSwitch"

AHLWAccessTestSwitch::AHLWAccessTestSwitch()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
		Mesh->SetRelativeScale3D(FVector(0.1f, 0.2f, 0.3f));
	}
}

bool AHLWAccessTestSwitch::IsActive() const
{
	if (Mode == EHLWTestSwitchMode::Lockdown)
	{
		const UHLWFacilityAccessSubsystem* Access = GetWorld()->GetSubsystem<UHLWFacilityAccessSubsystem>();
		return Access && Access->IsZoneLockedDown(LockdownZone);
	}

	// The first device stands for the group
	AHLWAccessDeviceBase* Device = Devices.IsEmpty() ? nullptr : Devices[0].Get();
	if (!Device)
	{
		return false;
	}

	switch (Mode)
	{
	case EHLWTestSwitchMode::Power:       return IHLWAccessDeviceState::Execute_IsPowered(Device);
	case EHLWTestSwitchMode::Network:     return IHLWAccessDeviceState::Execute_IsNetworked(Device);
	case EHLWTestSwitchMode::Operational: return IHLWAccessDeviceState::Execute_IsOperational(Device);
	case EHLWTestSwitchMode::AccessDisabled:
	{
		const AHLWKeycardReader* Reader = Cast<AHLWKeycardReader>(Device);
		return Reader && Reader->GetAccessData().bAccessDisabled;
	}
	default: return false;
	}
}

void AHLWAccessTestSwitch::Interact_Implementation(AActor* InteractingActor)
{
	const bool bNewState = !IsActive();

	if (Mode == EHLWTestSwitchMode::Lockdown)
	{
		if (UHLWFacilityAccessSubsystem* Access = GetWorld()->GetSubsystem<UHLWFacilityAccessSubsystem>())
		{
			Access->SetLockdown(LockdownZone, bNewState);
		}
		return;
	}

	for (AHLWAccessDeviceBase* Device : Devices)
	{
		if (!Device)
		{
			continue;
		}
		switch (Mode)
		{
		case EHLWTestSwitchMode::Power:       Device->SetPowered(bNewState); break;
		case EHLWTestSwitchMode::Network:     Device->SetNetworked(bNewState); break;
		case EHLWTestSwitchMode::Operational: Device->SetOperational(bNewState); break;
		case EHLWTestSwitchMode::AccessDisabled:
			if (AHLWKeycardReader* Reader = Cast<AHLWKeycardReader>(Device))
			{
				Reader->SetAccessDisabled(bNewState);
			}
			break;
		default: break;
		}
	}
}

FText AHLWAccessTestSwitch::GetInteractionText_Implementation()
{
	const bool bActive = IsActive();
	switch (Mode)
	{
	case EHLWTestSwitchMode::Power:          return bActive ? LOCTEXT("PowerOff", "Cut power") : LOCTEXT("PowerOn", "Restore power");
	case EHLWTestSwitchMode::Network:        return bActive ? LOCTEXT("NetOff", "Cut network") : LOCTEXT("NetOn", "Restore network");
	case EHLWTestSwitchMode::Operational:    return bActive ? LOCTEXT("Break", "Break reader") : LOCTEXT("Repair", "Repair reader");
	case EHLWTestSwitchMode::AccessDisabled: return bActive ? LOCTEXT("Enable", "Enable access") : LOCTEXT("Disable", "Disable access");
	case EHLWTestSwitchMode::Lockdown:       return bActive ? LOCTEXT("EndLockdown", "End lockdown") : LOCTEXT("StartLockdown", "Start lockdown");
	default:                                 return FText::GetEmpty();
	}
}

bool AHLWAccessTestSwitch::CanInteract_Implementation()
{
	return Mode == EHLWTestSwitchMode::Lockdown ? !LockdownZone.IsNone() : !Devices.IsEmpty();
}

#undef LOCTEXT_NAMESPACE
