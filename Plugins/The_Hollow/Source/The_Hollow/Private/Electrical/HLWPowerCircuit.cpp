#include "Electrical/HLWPowerCircuit.h"

#include "Access/HLWAccessTypes.h"
#include "Components/BillboardComponent.h"
#include "Components/TextRenderComponent.h"
#include "Electrical/HLWFacilityPowerSubsystem.h"
#include "Engine/World.h"

AHLWPowerCircuit::AHLWPowerCircuit()
{
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(GetRootComponent());
	Label->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(16.f);
	Label->SetHiddenInGame(true);

#if WITH_EDITORONLY_DATA
	Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (Sprite)
	{
		Sprite->SetupAttachment(GetRootComponent());
	}
#endif
}

void AHLWPowerCircuit::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const FString ID = CircuitData.CircuitID.IsNone() ? TEXT("(no ID)") : CircuitData.CircuitID.ToString();
	const FString Source = CircuitData.SourceID.IsNone() ? TEXT("(no source)") : CircuitData.SourceID.ToString();
	Label->SetText(FText::FromString(FString::Printf(TEXT("%s\n<- %s"), *ID, *Source)));
}

void AHLWPowerCircuit::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (CircuitData.CircuitID.IsNone())
	{
		CircuitData.CircuitID = HLWAccess::MakeStableID(this);
	}

	// Registered before any BeginPlay so the first resolve already sees it
	if (UHLWFacilityPowerSubsystem* Power = GetWorld() ? GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>() : nullptr)
	{
		Power->RegisterCircuit(CircuitData);
	}
}

FName AHLWPowerCircuit::GetCircuitID() const
{
	return CircuitData.CircuitID.IsNone() ? HLWAccess::MakeStableID(this) : CircuitData.CircuitID;
}

void AHLWPowerCircuit::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>())
	{
		Power->UnregisterCircuit(CircuitData.CircuitID);
	}
	Super::EndPlay(EndPlayReason);
}

void AHLWPowerCircuit::EnableCircuit()
{
	if (UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>())
	{
		Power->ActivateCircuit(CircuitData.CircuitID);
	}
}

void AHLWPowerCircuit::DisableCircuit()
{
	if (UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>())
	{
		Power->DeactivateCircuit(CircuitData.CircuitID);
	}
}

void AHLWPowerCircuit::SetCircuitFaulted(bool bFaulted)
{
	if (UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>())
	{
		Power->SetCircuitFaulted(CircuitData.CircuitID, bFaulted);
	}
}

EHLWPowerState AHLWPowerCircuit::GetCircuitPowerStatus() const
{
	const UHLWFacilityPowerSubsystem* Power = GetWorld() ? GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>() : nullptr;
	return Power ? Power->GetCircuitState(CircuitData.CircuitID) : EHLWPowerState::Unpowered;
}
