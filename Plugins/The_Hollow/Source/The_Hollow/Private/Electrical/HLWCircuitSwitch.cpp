#include "Electrical/HLWCircuitSwitch.h"

#include "Access/HLWAccessTypes.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Electrical/HLWElectricalFaultComponent.h"
#include "Electrical/HLWFacilityPowerSubsystem.h"
#include "Electrical/HLWPowerStateSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "HLWCircuitSwitch"

AHLWCircuitSwitch::AHLWCircuitSwitch()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	// Placeholder wall box, replace in the Blueprint
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
		Mesh->SetRelativeScale3D(FVector(0.1f, 0.3f, 0.4f));
	}

	StatusLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StatusLight"));
	StatusLight->SetupAttachment(Mesh);
	StatusLight->SetRelativeLocation(FVector(80.f, 0.f, 30.f));
	StatusLight->SetAttenuationRadius(120.f);
	StatusLight->SetCastShadows(false);
}

void AHLWCircuitSwitch::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Before the first power resolve, so the circuit starts in its final state
	LoadStartingState();
	ApplyToCircuit();
}

void AHLWCircuitSwitch::BeginPlay()
{
	Super::BeginPlay();

	if (CircuitID.IsNone())
	{
		UE_LOG(LogHLWPower, Warning, TEXT("%s has no CircuitID"), *GetName());
	}
	if (UHLWFacilityPowerSubsystem* Power = GetPowerSubsystem())
	{
		Power->OnCircuitStateChanged.AddDynamic(this, &ThisClass::HandleCircuitStateChanged);
	}
	if (UHLWPowerStateSubsystem* SaveState = UHLWPowerStateSubsystem::Get(this))
	{
		SaveState->OnStateRestored.AddDynamic(this, &ThisClass::HandleStateRestored);
	}
	UpdateLight();
}

void AHLWCircuitSwitch::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UHLWFacilityPowerSubsystem* Power = GetPowerSubsystem())
	{
		Power->OnCircuitStateChanged.RemoveDynamic(this, &ThisClass::HandleCircuitStateChanged);
	}
	if (UHLWPowerStateSubsystem* SaveState = UHLWPowerStateSubsystem::Get(this))
	{
		SaveState->OnStateRestored.RemoveDynamic(this, &ThisClass::HandleStateRestored);
	}
	Super::EndPlay(EndPlayReason);
}

void AHLWCircuitSwitch::LoadStartingState()
{
	const UHLWPowerStateSubsystem* SaveState = UHLWPowerStateSubsystem::Get(this);
	ReadSaveData(SaveState ? SaveState->FindSwitch(GetSwitchID()) : nullptr);
}

void AHLWCircuitSwitch::ReadSaveData(const FHLWSwitchSaveData* Saved)
{
	bOn = (Saved ? Saved->bOn : bStartsOn) && CanSwitchOn();
}

void AHLWCircuitSwitch::WriteSaveData(FHLWSwitchSaveData& Out) const
{
	Out.bOn = bOn;
}

void AHLWCircuitSwitch::SaveState() const
{
	if (UHLWPowerStateSubsystem* SaveState = UHLWPowerStateSubsystem::Get(this))
	{
		FHLWSwitchSaveData Data;
		WriteSaveData(Data);
		SaveState->SetSwitch(GetSwitchID(), Data);
	}
}

void AHLWCircuitSwitch::HandleStateRestored()
{
	LoadStartingState();
	ApplyToCircuit();
	UpdateLight();
}

bool AHLWCircuitSwitch::SetSwitchOn(bool bNewOn)
{
	if (bNewOn == bOn)
	{
		return true;
	}
	if (bNewOn && !CanSwitchOn())
	{
		return false;
	}

	bOn = bNewOn;
	ApplyToCircuit();
	SaveState();
	PlayOneShot(bOn ? SwitchOnSound.Get() : SwitchOffSound.Get());
	UpdateLight();
	OnSwitched.Broadcast(bOn);

	const UHLWFacilityPowerSubsystem* Power = GetPowerSubsystem();
	const bool bNoIncoming = bOn && Power && !Power->HasIncomingPower(CircuitID);
	Report(bNoIncoming
		? FString::Printf(TEXT("%s on: no incoming power"), *GetCircuitName().ToString())
		: FString::Printf(TEXT("%s %s"), *GetCircuitName().ToString(), bOn ? TEXT("on") : TEXT("off")));
	return true;
}

void AHLWCircuitSwitch::ApplyToCircuit() const
{
	if (UHLWFacilityPowerSubsystem* Power = GetPowerSubsystem())
	{
		Power->SetCircuitEnabled(CircuitID, bOn);
	}
}

void AHLWCircuitSwitch::HandleCircuitStateChanged(FName ChangedCircuitID, EHLWPowerState State, bool bInitial)
{
	if (ChangedCircuitID == CircuitID)
	{
		UpdateLight();
	}
}

void AHLWCircuitSwitch::UpdateLight()
{
	if (!bOn)
	{
		StatusLight->SetLightColor(OffColor);
	}
	else
	{
		// Predicts from upstream so the light is right before the next resolve
		const UHLWFacilityPowerSubsystem* Power = GetPowerSubsystem();
		const bool bLive = Power && Power->HasIncomingPower(CircuitID) && !Power->IsCircuitFaulted(CircuitID);
		StatusLight->SetLightColor(bLive ? LiveColor : NoPowerColor);
	}
	StatusLight->SetIntensity(LightIntensity);
}

void AHLWCircuitSwitch::Interact_Implementation(AActor* InteractingActor)
{
	if (UHLWElectricalFaultComponent* Fault = GetActiveFault())
	{
		Fault->TryRepair(InteractingActor);
		UpdateLight();
		return;
	}
	SetSwitchOn(!bOn);
}

FText AHLWCircuitSwitch::GetInteractionText_Implementation()
{
	if (const UHLWElectricalFaultComponent* Fault = GetActiveFault())
	{
		return Fault->GetRepairText();
	}
	return FText::Format(bOn ? LOCTEXT("Off", "Switch off {0}") : LOCTEXT("On", "Switch on {0}"), GetCircuitName());
}

bool AHLWCircuitSwitch::CanInteract_Implementation()
{
	if (const UHLWElectricalFaultComponent* Fault = GetActiveFault())
	{
		return Fault->CanRepair(UGameplayStatics::GetPlayerPawn(this, 0));
	}
	return bOn || CanSwitchOn();
}

FText AHLWCircuitSwitch::GetCircuitName() const
{
	const UHLWFacilityPowerSubsystem* Power = GetPowerSubsystem();
	return Power && !CircuitID.IsNone() ? Power->GetCircuitDisplayName(CircuitID) : LOCTEXT("Breaker", "breaker");
}

UHLWFacilityPowerSubsystem* AHLWCircuitSwitch::GetPowerSubsystem() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>() : nullptr;
}

FName AHLWCircuitSwitch::GetSwitchID() const
{
	return HLWAccess::MakeStableID(this);
}

UHLWElectricalFaultComponent* AHLWCircuitSwitch::GetActiveFault() const
{
	UHLWElectricalFaultComponent* Fault = FindComponentByClass<UHLWElectricalFaultComponent>();
	return Fault && Fault->IsFaultActive() ? Fault : nullptr;
}

void AHLWCircuitSwitch::PlayOneShot(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
	}
}

void AHLWCircuitSwitch::Report(const FString& Message) const
{
	UE_LOG(LogHLWPower, Log, TEXT("%s: %s"), *GetName(), *Message);
	if (bShowDebugResult && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, Message);
	}
}

#undef LOCTEXT_NAMESPACE
