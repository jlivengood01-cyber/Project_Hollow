#include "Electrical/HLWPowerGenerator.h"

#include "Access/HLWAccessTypes.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Electrical/HLWElectricalFaultComponent.h"
#include "Electrical/HLWFacilityPowerSubsystem.h"
#include "Electrical/HLWPowerStateSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "HLWPowerGenerator"

namespace
{
	constexpr float FuelTickInterval = 0.25f;
}

AHLWPowerGenerator::AHLWPowerGenerator()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	// Placeholder housing, replace in the Blueprint
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
		Mesh->SetRelativeScale3D(FVector(1.2f, 0.8f, 1.f));
	}

	StatusLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StatusLight"));
	StatusLight->SetupAttachment(Mesh);
	StatusLight->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	StatusLight->SetAttenuationRadius(250.f);
	StatusLight->SetCastShadows(false);

	Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	Audio->SetupAttachment(Mesh);
	Audio->bAutoActivate = false;
}

void AHLWPowerGenerator::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (GeneratorData.GeneratorID.IsNone())
	{
		GeneratorData.GeneratorID = HLWAccess::MakeStableID(this);
	}

	// Registered before any BeginPlay so the first resolve already sees it
	LoadStartingState();
	if (UHLWFacilityPowerSubsystem* Power = GetWorld() ? GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>() : nullptr)
	{
		Power->RegisterGenerator(this);
	}
}

void AHLWPowerGenerator::BeginPlay()
{
	Super::BeginPlay();

	if (UHLWPowerStateSubsystem* SaveState = UHLWPowerStateSubsystem::Get(this))
	{
		SaveState->OnStateRestored.AddDynamic(this, &ThisClass::HandleStateRestored);
	}
	StartStateTimers();
	UpdateFeedback();
}

void AHLWPowerGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	if (UHLWPowerStateSubsystem* SaveState = UHLWPowerStateSubsystem::Get(this))
	{
		SaveState->OnStateRestored.RemoveDynamic(this, &ThisClass::HandleStateRestored);
	}
	if (UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>())
	{
		Power->UnregisterGenerator(this);
	}
	Super::EndPlay(EndPlayReason);
}

FName AHLWPowerGenerator::GetGeneratorID() const
{
	return GeneratorData.GeneratorID.IsNone() ? HLWAccess::MakeStableID(this) : GeneratorData.GeneratorID;
}

void AHLWPowerGenerator::LoadStartingState()
{
	const UHLWPowerStateSubsystem* SaveState = UHLWPowerStateSubsystem::Get(this);
	if (const FHLWGeneratorSaveData* Saved = SaveState ? SaveState->FindGenerator(GetGeneratorID()) : nullptr)
	{
		State = Saved->State;
		Fuel = Saved->Fuel;
	}
	else
	{
		Fuel = GeneratorData.Fuel;
		State = GeneratorData.bRequiresFuel && Fuel <= 0.f ? EHLWGeneratorState::NoFuel
			: GeneratorData.bStartsRunning ? EHLWGeneratorState::Running
			: EHLWGeneratorState::Off;
	}

	if (GetActiveFault())
	{
		State = EHLWGeneratorState::Failed;
	}
}

void AHLWPowerGenerator::HandleStateRestored()
{
	// Quiet resync after a load, the power subsystem marks the next resolve as initial
	LoadStartingState();
	StartStateTimers();
	UpdateFeedback();
	OnStateChanged.Broadcast(GetGeneratorID(), State);
	if (UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>())
	{
		Power->NotifyGeneratorStateChanged(this);
	}
}

bool AHLWPowerGenerator::StartGenerator()
{
	if (State != EHLWGeneratorState::Off || !CheckGeneratorCondition())
	{
		return false;
	}

	PlayOneShot(StartSound);
	if (GeneratorData.StartupTime > 0.f)
	{
		SetState(EHLWGeneratorState::Starting);
	}
	else
	{
		FinishStartup();
	}
	return true;
}

void AHLWPowerGenerator::FinishStartup()
{
	SetState(EHLWGeneratorState::Running);
	Report(FString::Printf(TEXT("Generator %s running"), *GetGeneratorID().ToString()));
}

void AHLWPowerGenerator::StopGenerator()
{
	if (State != EHLWGeneratorState::Running && State != EHLWGeneratorState::Starting)
	{
		return;
	}
	PlayOneShot(StopSound);
	SetState(EHLWGeneratorState::Off);
	Report(FString::Printf(TEXT("Generator %s stopped"), *GetGeneratorID().ToString()));
}

bool AHLWPowerGenerator::CheckGeneratorCondition() const
{
	return State != EHLWGeneratorState::Failed && (!GeneratorData.bRequiresFuel || Fuel > 0.f);
}

float AHLWPowerGenerator::GetPowerOutput() const
{
	if (!IsRunning())
	{
		return 0.f;
	}
	return GeneratorData.Capacity > 0.f ? GeneratorData.Capacity : -1.f;
}

void AHLWPowerGenerator::TriggerGeneratorFailure()
{
	if (State == EHLWGeneratorState::Failed)
	{
		return;
	}
	PlayOneShot(FailSound);
	SetState(EHLWGeneratorState::Failed);
	Report(FString::Printf(TEXT("Generator %s failed"), *GetGeneratorID().ToString()));
}

void AHLWPowerGenerator::RepairGenerator()
{
	if (State != EHLWGeneratorState::Failed)
	{
		return;
	}
	SetState(GeneratorData.bRequiresFuel && Fuel <= 0.f ? EHLWGeneratorState::NoFuel : EHLWGeneratorState::Off);
	Report(FString::Printf(TEXT("Generator %s repaired"), *GetGeneratorID().ToString()));

	// A direct repair also clears the fault
	if (UHLWElectricalFaultComponent* Fault = GetActiveFault())
	{
		Fault->SetFaultActive(false);
	}
}

void AHLWPowerGenerator::AddFuel(float Amount)
{
	Fuel = FMath::Max(0.f, Fuel + Amount);
	if (State == EHLWGeneratorState::NoFuel && Fuel > 0.f)
	{
		SetState(EHLWGeneratorState::Off);
	}
	SaveState();
}

void AHLWPowerGenerator::DrainFuel()
{
	Fuel = FMath::Max(0.f, Fuel - GeneratorData.FuelPerSecond * FuelTickInterval);
	SaveState();
	if (Fuel <= 0.f)
	{
		PlayOneShot(StopSound);
		SetState(EHLWGeneratorState::NoFuel);
		Report(FString::Printf(TEXT("Generator %s out of fuel"), *GetGeneratorID().ToString()));
	}
}

void AHLWPowerGenerator::SetState(EHLWGeneratorState NewState)
{
	if (State == NewState)
	{
		return;
	}
	State = NewState;
	SaveState();
	StartStateTimers();
	UpdateFeedback();
	OnStateChanged.Broadcast(GetGeneratorID(), State);
	if (UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>())
	{
		Power->NotifyGeneratorStateChanged(this);
	}
}

void AHLWPowerGenerator::StartStateTimers()
{
	FTimerManager& Timers = GetWorldTimerManager();
	Timers.ClearTimer(StartupTimer);
	Timers.ClearTimer(FuelTimer);
	Timers.ClearTimer(RunTimer);

	if (State == EHLWGeneratorState::Starting)
	{
		Timers.SetTimer(StartupTimer, this, &ThisClass::FinishStartup, FMath::Max(GeneratorData.StartupTime, 0.01f), false);
	}
	else if (State == EHLWGeneratorState::Running)
	{
		if (GeneratorData.bRequiresFuel && GeneratorData.FuelPerSecond > 0.f)
		{
			Timers.SetTimer(FuelTimer, this, &ThisClass::DrainFuel, FuelTickInterval, true);
		}
		if (GeneratorData.RunTime > 0.f)
		{
			Timers.SetTimer(RunTimer, this, &ThisClass::StopGenerator, GeneratorData.RunTime, false);
		}
	}
}

void AHLWPowerGenerator::SaveState() const
{
	if (UHLWPowerStateSubsystem* SaveState = UHLWPowerStateSubsystem::Get(this))
	{
		SaveState->SetGenerator(GetGeneratorID(), State, Fuel);
	}
}

UHLWElectricalFaultComponent* AHLWPowerGenerator::GetActiveFault() const
{
	UHLWElectricalFaultComponent* Fault = FindComponentByClass<UHLWElectricalFaultComponent>();
	return Fault && Fault->IsFaultActive() ? Fault : nullptr;
}

void AHLWPowerGenerator::UpdateFeedback()
{
	switch (State)
	{
	case EHLWGeneratorState::Starting: StatusLight->SetLightColor(StartingColor); break;
	case EHLWGeneratorState::Running:  StatusLight->SetLightColor(RunningColor);  break;
	case EHLWGeneratorState::Failed:
	case EHLWGeneratorState::NoFuel:   StatusLight->SetLightColor(FailedColor);   break;
	default: break;
	}
	StatusLight->SetIntensity(State == EHLWGeneratorState::Off ? 0.f : LightIntensity);

	if (State == EHLWGeneratorState::Running && RunningSound)
	{
		Audio->SetSound(RunningSound);
		Audio->Play();
	}
	else
	{
		Audio->Stop();
	}
}

void AHLWPowerGenerator::Interact_Implementation(AActor* InteractingActor)
{
	switch (State)
	{
	case EHLWGeneratorState::Running:
		StopGenerator();
		break;
	case EHLWGeneratorState::Off:
		StartGenerator();
		break;
	case EHLWGeneratorState::Failed:
		if (!bPlayerCanRepair)
		{
			break;
		}
		if (UHLWElectricalFaultComponent* Fault = GetActiveFault())
		{
			Fault->TryRepair(InteractingActor);
		}
		else
		{
			RepairGenerator();
		}
		break;
	default:
		break;
	}
}

FText AHLWPowerGenerator::GetInteractionText_Implementation()
{
	switch (State)
	{
	case EHLWGeneratorState::Off:      return LOCTEXT("Start", "Start generator");
	case EHLWGeneratorState::Starting: return LOCTEXT("Starting", "Starting...");
	case EHLWGeneratorState::Running:  return LOCTEXT("Stop", "Stop generator");
	case EHLWGeneratorState::Failed:
		if (!bPlayerCanRepair)
		{
			return LOCTEXT("Failed", "Generator damaged");
		}
		if (const UHLWElectricalFaultComponent* Fault = GetActiveFault())
		{
			return Fault->GetRepairText();
		}
		return LOCTEXT("Repair", "Repair generator");
	case EHLWGeneratorState::NoFuel:   return LOCTEXT("NoFuel", "Out of fuel");
	default:                           return FText::GetEmpty();
	}
}

bool AHLWPowerGenerator::CanInteract_Implementation()
{
	if (State == EHLWGeneratorState::Failed)
	{
		const UHLWElectricalFaultComponent* Fault = GetActiveFault();
		return bPlayerCanRepair && (!Fault || Fault->CanRepair(UGameplayStatics::GetPlayerPawn(this, 0)));
	}
	return bPlayerCanOperate && (State == EHLWGeneratorState::Off || State == EHLWGeneratorState::Running);
}

void AHLWPowerGenerator::PlayOneShot(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
	}
}

void AHLWPowerGenerator::Report(const FString& Message) const
{
	UE_LOG(LogHLWPower, Log, TEXT("%s"), *Message);
	if (bShowDebugResult && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, Message);
	}
}

#undef LOCTEXT_NAMESPACE
