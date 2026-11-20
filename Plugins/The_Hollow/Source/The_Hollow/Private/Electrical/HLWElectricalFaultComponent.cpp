#include "Electrical/HLWElectricalFaultComponent.h"

#include "Access/HLWAccessTypes.h"
#include "Electrical/HLWCircuitSwitch.h"
#include "Electrical/HLWFacilityPowerSubsystem.h"
#include "Electrical/HLWItemHolderComponent.h"
#include "Electrical/HLWPowerCircuit.h"
#include "Electrical/HLWPowerGenerator.h"
#include "Electrical/HLWPowerStateSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "HLWElectricalFault"

UHLWElectricalFaultComponent::UHLWElectricalFaultComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

void UHLWElectricalFaultComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// Runs before the first power resolve, so the starting fault is silent
	const UHLWPowerStateSubsystem* SaveState = UHLWPowerStateSubsystem::Get(this);
	const bool* Saved = SaveState ? SaveState->FindFault(GetFaultID()) : nullptr;
	bFaultActive = Saved ? *Saved : FaultData.bActive;
	ApplyCircuitFault();
}

void UHLWElectricalFaultComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UHLWPowerStateSubsystem* SaveState = UHLWPowerStateSubsystem::Get(this))
	{
		SaveState->OnStateRestored.AddDynamic(this, &ThisClass::HandleStateRestored);
	}
	if (GetTargetID().IsNone())
	{
		UE_LOG(LogHLWPower, Warning, TEXT("%s on %s has no target"), *GetName(), *GetOwner()->GetName());
	}
}

void UHLWElectricalFaultComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UHLWPowerStateSubsystem* SaveState = UHLWPowerStateSubsystem::Get(this))
	{
		SaveState->OnStateRestored.RemoveDynamic(this, &ThisClass::HandleStateRestored);
	}
	if (!TargetsOwnerGenerator())
	{
		if (UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>())
		{
			Power->SetCircuitFaultSource(GetTargetID(), GetFaultID(), false);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void UHLWElectricalFaultComponent::HandleStateRestored()
{
	const UHLWPowerStateSubsystem* SaveState = UHLWPowerStateSubsystem::Get(this);
	const bool* Saved = SaveState ? SaveState->FindFault(GetFaultID()) : nullptr;
	bFaultActive = Saved ? *Saved : FaultData.bActive;

	// Generators restore their own failed state
	ApplyCircuitFault();
	OnFaultChanged.Broadcast(bFaultActive);
}

void UHLWElectricalFaultComponent::SetFaultActive(bool bActive)
{
	if (bFaultActive == bActive)
	{
		return;
	}
	bFaultActive = bActive;

	if (UHLWPowerStateSubsystem* SaveState = UHLWPowerStateSubsystem::Get(this))
	{
		SaveState->SetFault(GetFaultID(), bFaultActive);
	}

	if (TargetsOwnerGenerator())
	{
		AHLWPowerGenerator* Generator = CastChecked<AHLWPowerGenerator>(GetOwner());
		if (bFaultActive)
		{
			Generator->TriggerGeneratorFailure();
		}
		else
		{
			Generator->RepairGenerator();
		}
	}
	else
	{
		ApplyCircuitFault();
	}

	if (bFaultActive)
	{
		PlayOneShot(FaultSound);
	}
	OnFaultChanged.Broadcast(bFaultActive);
	if (UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>())
	{
		Power->NotifyElectricalFault(GetFaultID(), GetTargetID(), FaultData.FaultType, bFaultActive);
	}
	Report(FString::Printf(TEXT("%s %s: %s"),
		*UEnum::GetDisplayValueAsText(FaultData.FaultType).ToString(),
		bFaultActive ? TEXT("fault") : TEXT("repaired"),
		*GetTargetID().ToString()));
}

bool UHLWElectricalFaultComponent::TryRepair(AActor* Repairer)
{
	if (!bFaultActive)
	{
		return false;
	}
	if (!CanRepair(Repairer))
	{
		Report(GetRepairText().ToString());
		return false;
	}

	if (!FaultData.RepairItem.IsNone() && bConsumeRepairItem)
	{
		UHLWItemHolderComponent* Holder = Repairer->FindComponentByClass<UHLWItemHolderComponent>();
		if (!Holder || !Holder->RemoveItem(FaultData.RepairItem))
		{
			return false;
		}
	}

	PlayOneShot(RepairSound);
	SetFaultActive(false);
	return true;
}

bool UHLWElectricalFaultComponent::CanRepair(const AActor* Repairer) const
{
	if (FaultData.RepairItem.IsNone())
	{
		return true;
	}
	const UHLWItemHolderComponent* Holder = Repairer ? Repairer->FindComponentByClass<UHLWItemHolderComponent>() : nullptr;
	return Holder && Holder->HasItem(FaultData.RepairItem);
}

FText UHLWElectricalFaultComponent::GetRepairText() const
{
	// Single player, so the prompt checks player 0
	if (CanRepair(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		return FText::Format(LOCTEXT("Repair", "Repair {0}"), UEnum::GetDisplayValueAsText(FaultData.FaultType));
	}
	const FText Item = RepairItemName.IsEmpty() ? FText::FromName(FaultData.RepairItem) : RepairItemName;
	return FText::Format(LOCTEXT("Needs", "Needs {0}"), Item);
}

FName UHLWElectricalFaultComponent::GetTargetID() const
{
	if (!FaultData.TargetID.IsNone())
	{
		return FaultData.TargetID;
	}

	const AActor* Owner = GetOwner();
	if (const AHLWPowerGenerator* Generator = Cast<AHLWPowerGenerator>(Owner))
	{
		return Generator->GetGeneratorID();
	}
	if (const AHLWPowerCircuit* Circuit = Cast<AHLWPowerCircuit>(Owner))
	{
		return Circuit->GetCircuitID();
	}
	if (const AHLWCircuitSwitch* Switch = Cast<AHLWCircuitSwitch>(Owner))
	{
		return Switch->GetCircuitID();
	}
	return NAME_None;
}

FName UHLWElectricalFaultComponent::GetFaultID() const
{
	return FName(*FString::Printf(TEXT("%s.%s"), *HLWAccess::MakeStableID(GetOwner()).ToString(), *GetName()));
}

bool UHLWElectricalFaultComponent::TargetsOwnerGenerator() const
{
	const AHLWPowerGenerator* Generator = Cast<AHLWPowerGenerator>(GetOwner());
	return Generator && (FaultData.TargetID.IsNone() || FaultData.TargetID == Generator->GetGeneratorID());
}

void UHLWElectricalFaultComponent::ApplyCircuitFault() const
{
	if (TargetsOwnerGenerator())
	{
		return;
	}
	if (UHLWFacilityPowerSubsystem* Power = GetWorld() ? GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>() : nullptr)
	{
		Power->SetCircuitFaultSource(GetTargetID(), GetFaultID(), bFaultActive);
	}
}

void UHLWElectricalFaultComponent::PlayOneShot(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetOwner()->GetActorLocation());
	}
}

void UHLWElectricalFaultComponent::Report(const FString& Message) const
{
	UE_LOG(LogHLWPower, Log, TEXT("%s"), *Message);
	if (bShowDebugResult && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, Message);
	}
}

#undef LOCTEXT_NAMESPACE
