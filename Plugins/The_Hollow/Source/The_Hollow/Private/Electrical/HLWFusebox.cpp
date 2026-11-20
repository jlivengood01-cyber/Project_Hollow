#include "Electrical/HLWFusebox.h"

#include "Electrical/HLWFacilityPowerSubsystem.h"
#include "Electrical/HLWItemHolderComponent.h"
#include "Electrical/HLWPowerStateSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "HLWFusebox"

void AHLWFusebox::ReadSaveData(const FHLWSwitchSaveData* Saved)
{
	if (Saved)
	{
		InstalledFuseType = Saved->InstalledFuse;
		bInspected = Saved->bInspected;
		bFaultedNow = Saved->bFaulted;
	}
	else
	{
		InstalledFuseType = bStartsWithFuse && !AcceptedFuseTypes.IsEmpty() ? AcceptedFuseTypes[0] : NAME_None;
		bInspected = false;
		bFaultedNow = bFaulted;
	}

	// Base reads the switch position last, so it respects the fuse and fault
	Super::ReadSaveData(Saved);
}

void AHLWFusebox::WriteSaveData(FHLWSwitchSaveData& Out) const
{
	Super::WriteSaveData(Out);
	Out.InstalledFuse = InstalledFuseType;
	Out.bInspected = bInspected;
	Out.bFaulted = bFaultedNow;
}

void AHLWFusebox::LoadStartingState()
{
	Super::LoadStartingState();
	ApplyFault();
}

void AHLWFusebox::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UHLWFacilityPowerSubsystem* Power = GetPowerSubsystem())
	{
		Power->SetCircuitFaultSource(CircuitID, GetSwitchID(), false);
	}
	Super::EndPlay(EndPlayReason);
}

void AHLWFusebox::ApplyFault() const
{
	if (UHLWFacilityPowerSubsystem* Power = GetPowerSubsystem())
	{
		Power->SetCircuitFaultSource(CircuitID, GetSwitchID(), bFaultedNow);
	}
}

EHLWFuseboxState AHLWFusebox::GetFuseboxState() const
{
	if (bFaultedNow)
	{
		return EHLWFuseboxState::Faulted;
	}
	if (!HasFuse())
	{
		return EHLWFuseboxState::MissingFuse;
	}
	return bOn ? EHLWFuseboxState::On : EHLWFuseboxState::Off;
}

bool AHLWFusebox::InstallFuse(AActor* FromActor)
{
	if (HasFuse() || bFaultedNow || !FromActor)
	{
		return false;
	}

	const FName FuseType = FindHeldFuse(FromActor);
	UHLWItemHolderComponent* Holder = FromActor->FindComponentByClass<UHLWItemHolderComponent>();
	if (FuseType.IsNone() || !Holder || !Holder->RemoveItem(FuseType))
	{
		return false;
	}

	InstalledFuseType = FuseType;
	SaveState();
	PlayOneShot(InstallSound);
	UpdateLight();
	OnFuseInstalled.Broadcast(FuseType);
	Report(FString::Printf(TEXT("Installed %s"), *FuseType.ToString()));
	return true;
}

void AHLWFusebox::SetFaulted(bool bNewFaulted)
{
	if (bFaultedNow == bNewFaulted)
	{
		return;
	}
	bFaultedNow = bNewFaulted;
	if (bFaultedNow)
	{
		SetSwitchOn(false);
	}
	ApplyFault();
	SaveState();
	UpdateLight();
}

void AHLWFusebox::Interact_Implementation(AActor* InteractingActor)
{
	if (bFaultedNow)
	{
		return;
	}
	if (HasFuse() || GetActiveFault())
	{
		Super::Interact_Implementation(InteractingActor);
		return;
	}
	if (InstallFuse(InteractingActor))
	{
		return;
	}

	// No fuse to install: this is the inspect step
	bInspected = true;
	SaveState();
	OnInspected.Broadcast();
	Report(FString::Printf(TEXT("Fusebox needs a fuse (%s)"),
		AcceptedFuseTypes.IsEmpty() ? TEXT("none accepted") : *AcceptedFuseTypes[0].ToString()));
}

FText AHLWFusebox::GetInteractionText_Implementation()
{
	if (bFaultedNow)
	{
		return LOCTEXT("Faulted", "Fusebox damaged");
	}
	if (!HasFuse() && !GetActiveFault())
	{
		if (PlayerHoldsFuse())
		{
			return LOCTEXT("Install", "Install fuse");
		}
		return bInspected ? LOCTEXT("NeedsFuse", "Needs a fuse") : LOCTEXT("Inspect", "Inspect fusebox");
	}
	return Super::GetInteractionText_Implementation();
}

bool AHLWFusebox::CanInteract_Implementation()
{
	if (bFaultedNow)
	{
		return false;
	}
	if (!HasFuse() && !GetActiveFault())
	{
		return !bInspected || PlayerHoldsFuse();
	}
	return Super::CanInteract_Implementation();
}

FName AHLWFusebox::FindHeldFuse(const AActor* Holder) const
{
	const UHLWItemHolderComponent* Items = Holder ? Holder->FindComponentByClass<UHLWItemHolderComponent>() : nullptr;
	if (Items)
	{
		for (const FName& Type : AcceptedFuseTypes)
		{
			if (Items->HasItem(Type))
			{
				return Type;
			}
		}
	}
	return NAME_None;
}

bool AHLWFusebox::PlayerHoldsFuse() const
{
	// Single player, so the prompt checks player 0
	return !FindHeldFuse(UGameplayStatics::GetPlayerPawn(this, 0)).IsNone();
}

#undef LOCTEXT_NAMESPACE
