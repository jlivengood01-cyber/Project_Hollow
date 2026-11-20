#include "Electrical/HLWPowerConsumerComponent.h"

#include "Electrical/HLWFacilityPowerSubsystem.h"
#include "Engine/World.h"

UHLWPowerConsumerComponent::UHLWPowerConsumerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHLWPowerConsumerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UsesCircuit())
	{
		// Dark until the first update arrives
		bPowered = false;
		CircuitState = EHLWPowerState::Unpowered;
		Register();
	}
}

void UHLWPowerConsumerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Unregister();
	Super::EndPlay(EndPlayReason);
}

void UHLWPowerConsumerComponent::SetCircuitID(FName NewCircuitID)
{
	if (NewCircuitID == ConsumerData.CircuitID)
	{
		return;
	}

	Unregister();
	ConsumerData.CircuitID = NewCircuitID;

	if (!HasBegunPlay())
	{
		return;
	}
	if (UsesCircuit())
	{
		Register();
	}
	else
	{
		CircuitState = EHLWPowerState::Powered;
		if (!bPowered)
		{
			SetPowered(true, false);
		}
	}
}

void UHLWPowerConsumerComponent::ReceivePowerUpdate(EHLWPowerState NewCircuitState, bool bInitial)
{
	// The first update always goes out so owners can sync
	const bool bFirst = !bHasReceivedState;
	const bool bStateChanged = bFirst || NewCircuitState != CircuitState;
	const bool bNewPowered = HLWPower::SuppliesConsumer(NewCircuitState, ConsumerData.bEmergencyLoad);
	const bool bPowerChanged = bFirst || bNewPowered != bPowered;

	CircuitState = NewCircuitState;
	bHasReceivedState = true;

	if (bPowerChanged)
	{
		SetPowered(bNewPowered, bInitial || bFirst);
	}
	if (bStateChanged)
	{
		OnCircuitStateChanged.Broadcast(CircuitState, bInitial || bFirst);
	}
}

void UHLWPowerConsumerComponent::SetPowered(bool bNewPowered, bool bInitial)
{
	bPowered = bNewPowered;
	OnPowerChanged.Broadcast(bPowered, bInitial);
	if (bPowered)
	{
		OnPowerRestored.Broadcast(bInitial);
	}
	else
	{
		OnPowerLost.Broadcast(bInitial);
	}
}

void UHLWPowerConsumerComponent::Register()
{
	if (UHLWFacilityPowerSubsystem* Power = GetWorld() ? GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>() : nullptr)
	{
		Power->RegisterConsumer(this);
	}
}

void UHLWPowerConsumerComponent::Unregister()
{
	if (UHLWFacilityPowerSubsystem* Power = GetWorld() ? GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>() : nullptr)
	{
		Power->UnregisterConsumer(this);
	}
	bHasReceivedState = false;
}
