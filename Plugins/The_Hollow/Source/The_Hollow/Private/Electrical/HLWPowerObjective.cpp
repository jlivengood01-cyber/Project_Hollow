#include "Electrical/HLWPowerObjective.h"

#include "Access/HLWAccessTypes.h"
#include "Electrical/HLWFacilityPowerSubsystem.h"
#include "Electrical/HLWFusebox.h"
#include "Electrical/HLWPowerGenerator.h"
#include "Electrical/HLWPowerStateSubsystem.h"
#include "Electrical/HLWPoweredMachine.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "HLWPowerObjective"

AHLWPowerObjective::AHLWPowerObjective()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	ObjectiveName = LOCTEXT("DefaultName", "Restore Power to the Research Section");
	StageTexts.Add(EHLWPowerObjectiveStage::FindGenerator, LOCTEXT("FindGenerator", "Find the generator and start it."));
	StageTexts.Add(EHLWPowerObjectiveStage::InspectPanel, LOCTEXT("InspectPanel", "Inspect the section's fusebox."));
	StageTexts.Add(EHLWPowerObjectiveStage::FindFuse, LOCTEXT("FindFuse", "Find a replacement fuse."));
	StageTexts.Add(EHLWPowerObjectiveStage::InstallFuse, LOCTEXT("InstallFuse", "Install the fuse."));
	StageTexts.Add(EHLWPowerObjectiveStage::ActivateFusebox, LOCTEXT("ActivateFusebox", "Switch on the fusebox."));
	StageTexts.Add(EHLWPowerObjectiveStage::RestoreCircuit, LOCTEXT("RestoreCircuit", "No power is reaching the section. Check the generator."));
	StageTexts.Add(EHLWPowerObjectiveStage::ConfirmEquipment, LOCTEXT("ConfirmEquipment", "Check the research equipment."));
	StageTexts.Add(EHLWPowerObjectiveStage::Complete, LOCTEXT("Complete", "Power restored."));
}

void AHLWPowerObjective::BeginPlay()
{
	Super::BeginPlay();

	ObjectiveID = HLWAccess::MakeStableID(this);

	if (UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>())
	{
		Power->OnGeneratorStateChanged.AddDynamic(this, &ThisClass::HandleGeneratorStateChanged);
		Power->OnCircuitStateChanged.AddDynamic(this, &ThisClass::HandleCircuitStateChanged);
		Power->OnFuseCollected.AddDynamic(this, &ThisClass::HandleFuseCollected);
	}
	if (TargetFusebox)
	{
		TargetFusebox->OnInspected.AddDynamic(this, &ThisClass::HandleFuseboxInspected);
		TargetFusebox->OnFuseInstalled.AddDynamic(this, &ThisClass::HandleFuseInstalled);
		TargetFusebox->OnSwitched.AddDynamic(this, &ThisClass::HandleFuseboxSwitched);
	}
	if (ConfirmEquipment)
	{
		ConfirmEquipment->OnInspected.AddDynamic(this, &ThisClass::HandleEquipmentInspected);
	}

	UHLWPowerStateSubsystem* State = UHLWPowerStateSubsystem::Get(this);
	const uint8* Saved = State ? State->FindObjectiveStage(ObjectiveID) : nullptr;
	if (State)
	{
		State->OnStateRestored.AddDynamic(this, &ThisClass::HandleStateRestored);
	}

	if (Saved)
	{
		SetStage(static_cast<EHLWPowerObjectiveStage>(*Saved), false);
	}
	else if (bStartActive)
	{
		// Next tick so every target actor has begun play
		GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::Activate);
	}
}

void AHLWPowerObjective::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	if (UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>())
	{
		Power->OnGeneratorStateChanged.RemoveDynamic(this, &ThisClass::HandleGeneratorStateChanged);
		Power->OnCircuitStateChanged.RemoveDynamic(this, &ThisClass::HandleCircuitStateChanged);
		Power->OnFuseCollected.RemoveDynamic(this, &ThisClass::HandleFuseCollected);
	}
	if (UHLWPowerStateSubsystem* State = UHLWPowerStateSubsystem::Get(this))
	{
		State->OnStateRestored.RemoveDynamic(this, &ThisClass::HandleStateRestored);
	}
	Super::EndPlay(EndPlayReason);
}

void AHLWPowerObjective::Activate()
{
	if (Stage == EHLWPowerObjectiveStage::Inactive)
	{
		SetStage(EHLWPowerObjectiveStage::FindGenerator, true);
		Advance();
	}
}

FText AHLWPowerObjective::GetStageText() const
{
	const FText* Found = StageTexts.Find(Stage);
	return Found ? *Found : FText::GetEmpty();
}

FName AHLWPowerObjective::GetCircuitID() const
{
	if (!TargetCircuitID.IsNone())
	{
		return TargetCircuitID;
	}
	return TargetFusebox ? TargetFusebox->GetCircuitID() : NAME_None;
}

bool AHLWPowerObjective::IsStageDone(EHLWPowerObjectiveStage Check) const
{
	const UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>();
	const FName CircuitID = GetCircuitID();
	const bool bHasFuse = TargetFusebox && TargetFusebox->HasFuse();

	// The resolve runs a frame after a switch flips, so predict from upstream
	const bool bSwitchedLive = TargetFusebox && TargetFusebox->IsSwitchOn() && Power
		&& Power->HasIncomingPower(CircuitID) && !Power->IsCircuitFaulted(CircuitID);
	const bool bCircuitPowered = (Power && Power->IsCircuitPowered(CircuitID)) || bSwitchedLive;

	switch (Check)
	{
	case EHLWPowerObjectiveStage::FindGenerator:
		return !TargetGenerator || TargetGenerator->IsRunning() || bCircuitPowered;
	case EHLWPowerObjectiveStage::InspectPanel:
		return !TargetFusebox || TargetFusebox->WasInspected() || bHasFuse;
	case EHLWPowerObjectiveStage::FindFuse:
		// Single player, so the fuse check looks at player 0
		return !TargetFusebox || bHasFuse || !TargetFusebox->FindHeldFuse(UGameplayStatics::GetPlayerPawn(this, 0)).IsNone();
	case EHLWPowerObjectiveStage::InstallFuse:
		return !TargetFusebox || bHasFuse;
	case EHLWPowerObjectiveStage::ActivateFusebox:
		return !TargetFusebox || TargetFusebox->IsSwitchOn();
	case EHLWPowerObjectiveStage::RestoreCircuit:
		return bCircuitPowered;
	case EHLWPowerObjectiveStage::ConfirmEquipment:
		return !ConfirmEquipment || bEquipmentConfirmed;
	default:
		return false;
	}
}

void AHLWPowerObjective::Advance()
{
	if (Stage == EHLWPowerObjectiveStage::Inactive || Stage == EHLWPowerObjectiveStage::Complete)
	{
		return;
	}

	EHLWPowerObjectiveStage Next = Stage;
	while (Next != EHLWPowerObjectiveStage::Complete && IsStageDone(Next))
	{
		Next = static_cast<EHLWPowerObjectiveStage>(static_cast<uint8>(Next) + 1);
	}
	if (Next != Stage)
	{
		SetStage(Next, true);
	}
}

void AHLWPowerObjective::HandleGeneratorStateChanged(FName GeneratorID, EHLWGeneratorState State)
{
	Advance();
}

void AHLWPowerObjective::HandleCircuitStateChanged(FName CircuitID, EHLWPowerState State, bool bInitial)
{
	if (CircuitID == GetCircuitID())
	{
		Advance();
	}
}

void AHLWPowerObjective::HandleFuseCollected(FName FuseType, AActor* Collector)
{
	Advance();
}

void AHLWPowerObjective::HandleFuseboxInspected()
{
	Advance();
}

void AHLWPowerObjective::HandleFuseInstalled(FName FuseType)
{
	Advance();
}

void AHLWPowerObjective::HandleFuseboxSwitched(bool bOn)
{
	Advance();
}

void AHLWPowerObjective::HandleEquipmentInspected(bool bPowered)
{
	if (bPowered && Stage == EHLWPowerObjectiveStage::ConfirmEquipment)
	{
		bEquipmentConfirmed = true;
		Advance();
	}
}

void AHLWPowerObjective::HandleStateRestored()
{
	const UHLWPowerStateSubsystem* State = UHLWPowerStateSubsystem::Get(this);
	const uint8* Saved = State ? State->FindObjectiveStage(ObjectiveID) : nullptr;
	bEquipmentConfirmed = false;
	SetStage(Saved ? static_cast<EHLWPowerObjectiveStage>(*Saved) : EHLWPowerObjectiveStage::Inactive, false);

	if (!Saved && bStartActive)
	{
		Activate();
	}
}

void AHLWPowerObjective::SetStage(EHLWPowerObjectiveStage NewStage, bool bAnnounce)
{
	Stage = NewStage;

	if (UHLWPowerStateSubsystem* State = UHLWPowerStateSubsystem::Get(this))
	{
		State->SetObjectiveStage(ObjectiveID, static_cast<uint8>(Stage));
	}
	OnStageChanged.Broadcast(Stage);

	if (bAnnounce && bShowDebugText && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, FString::Printf(TEXT("%s: %s"), *ObjectiveName.ToString(), *GetStageText().ToString()));
	}
}

#undef LOCTEXT_NAMESPACE
