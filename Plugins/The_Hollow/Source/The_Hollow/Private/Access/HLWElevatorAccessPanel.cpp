#include "Access/HLWElevatorAccessPanel.h"

#include "Access/HLWFacilityAccessSubsystem.h"
#include "Access/HLWKeycardInventoryComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "HLWElevatorAccessPanel"

AHLWElevatorAccessPanel::AHLWElevatorAccessPanel()
{
	Mesh->SetRelativeScale3D(FVector(0.05f, 0.3f, 0.5f));
}

void AHLWElevatorAccessPanel::BeginPlay()
{
	Super::BeginPlay();

	// Give each restricted floor its own stable ID for saves and events
	const FString PanelID = HLWAccess::MakeStableID(this).ToString();
	for (int32 Index = 0; Index < Floors.Num(); ++Index)
	{
		FHLWFacilityAccessData& Access = Floors[Index].Access;
		Access.DeviceType = EHLWAccessDeviceType::Elevator;
		if (Access.DeviceID.IsNone())
		{
			Access.DeviceID = FName(*FString::Printf(TEXT("%s.Floor%d"), *PanelID, Index));
		}
	}
}

EHLWAccessResult AHLWElevatorAccessPanel::CheckFloor(int32 FloorIndex, AActor* User) const
{
	return EvaluateFloor(FloorIndex, User, false);
}

EHLWAccessResult AHLWElevatorAccessPanel::RequestFloor(int32 FloorIndex, AActor* User)
{
	const EHLWAccessResult Result = EvaluateFloor(FloorIndex, User, true);

	if (Result == EHLWAccessResult::Granted)
	{
		OnFloorAuthorized.Broadcast(FloorIndex);
		if (bArriveInstantly)
		{
			SetCurrentFloor(FloorIndex);
		}
	}
	else
	{
		OnFloorDenied.Broadcast(FloorIndex, Result);
	}

	if (bShowDebugResult && GEngine && Floors.IsValidIndex(FloorIndex))
	{
		const FColor Color = Result == EHLWAccessResult::Granted ? FColor::Green : FColor::Red;
		GEngine->AddOnScreenDebugMessage(-1, 3.f, Color, FString::Printf(TEXT("Elevator %s: %s"),
			*Floors[FloorIndex].FloorName.ToString(), *UEnum::GetDisplayValueAsText(Result).ToString()));
	}
	return Result;
}

EHLWAccessResult AHLWElevatorAccessPanel::EvaluateFloor(int32 FloorIndex, AActor* User, bool bBroadcast) const
{
	if (!Floors.IsValidIndex(FloorIndex))
	{
		return EHLWAccessResult::InvalidDevice;
	}

	UHLWFacilityAccessSubsystem* Access = GetWorld()->GetSubsystem<UHLWFacilityAccessSubsystem>();
	const FHLWElevatorFloor& Floor = Floors[FloorIndex];

	// Elevator power and faults are checked apart from authorization
	EHLWAccessResult PowerResult = EHLWAccessResult::Granted;
	if (!bPowered)
	{
		PowerResult = EHLWAccessResult::NoPower;
	}
	else if (!bOperational)
	{
		PowerResult = EHLWAccessResult::DeviceOffline;
	}
	if (PowerResult != EHLWAccessResult::Granted)
	{
		if (bBroadcast && Access)
		{
			Access->ReportAccessResult(Floor.Access.DeviceID, PowerResult);
		}
		return PowerResult;
	}

	if (!Floor.bRestricted)
	{
		return EHLWAccessResult::Granted;
	}
	if (!Access)
	{
		return EHLWAccessResult::InvalidDevice;
	}

	const UHLWKeycardInventoryComponent* Inventory = User ? User->FindComponentByClass<UHLWKeycardInventoryComponent>() : nullptr;
	return bBroadcast
		? Access->RequestAccess(Inventory, Floor.Access, nullptr)
		: Access->CheckAccess(Inventory, Floor.Access, nullptr);
}

void AHLWElevatorAccessPanel::SetCurrentFloor(int32 FloorIndex)
{
	if (Floors.IsValidIndex(FloorIndex))
	{
		CurrentFloor = FloorIndex;
	}
}

int32 AHLWElevatorAccessPanel::GetNextFloor() const
{
	return Floors.IsEmpty() ? INDEX_NONE : (CurrentFloor + 1) % Floors.Num();
}

void AHLWElevatorAccessPanel::Interact_Implementation(AActor* InteractingActor)
{
	RequestFloor(GetNextFloor(), InteractingActor);
}

FText AHLWElevatorAccessPanel::GetInteractionText_Implementation()
{
	const int32 Next = GetNextFloor();
	if (!Floors.IsValidIndex(Next))
	{
		return FText::GetEmpty();
	}

	const FHLWElevatorFloor& Floor = Floors[Next];
	return Floor.bRestricted
		? FText::Format(LOCTEXT("GoRestricted", "Go to {0} (restricted)"), Floor.FloorName)
		: FText::Format(LOCTEXT("Go", "Go to {0}"), Floor.FloorName);
}

bool AHLWElevatorAccessPanel::CanInteract_Implementation()
{
	return Floors.Num() > 1;
}

#undef LOCTEXT_NAMESPACE
