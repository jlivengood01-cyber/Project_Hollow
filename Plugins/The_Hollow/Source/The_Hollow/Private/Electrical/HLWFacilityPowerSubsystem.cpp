#include "Electrical/HLWFacilityPowerSubsystem.h"

#include "Electrical/HLWPowerConsumerComponent.h"
#include "Electrical/HLWPowerGenerator.h"
#include "Electrical/HLWPowerStateSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarPowerDebug(
		TEXT("HLW.Power.Debug"), 0,
		TEXT("1 = list generators and circuits on screen"));

	// Guards against a generator failing and restarting in the same frame forever
	constexpr int32 MaxResolvePasses = 4;
}

bool UHLWFacilityPowerSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UHLWFacilityPowerSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	bWorldStarted = true;

	if (UHLWPowerStateSubsystem* SaveState = UHLWPowerStateSubsystem::Get(this))
	{
		// Switches already applied their own saved state, script overrides fill the rest
		for (const TPair<FName, bool>& Saved : SaveState->GetCircuitEnabled())
		{
			if (!EnabledOverrides.Contains(Saved.Key))
			{
				EnabledOverrides.Add(Saved.Key, Saved.Value);
			}
		}
		SaveState->OnStateRestored.AddDynamic(this, &ThisClass::HandleStateRestored);
	}

	// Actors registered in PostInitializeComponents, so this runs before any BeginPlay
	UpdatePowerAvailability();
}

void UHLWFacilityPowerSubsystem::Deinitialize()
{
	if (UHLWPowerStateSubsystem* SaveState = UHLWPowerStateSubsystem::Get(this))
	{
		SaveState->OnStateRestored.RemoveDynamic(this, &ThisClass::HandleStateRestored);
	}
	Super::Deinitialize();
}

void UHLWFacilityPowerSubsystem::HandleStateRestored()
{
	if (const UHLWPowerStateSubsystem* SaveState = UHLWPowerStateSubsystem::Get(this))
	{
		EnabledOverrides = SaveState->GetCircuitEnabled();
	}
	bNextResolveSilent = true;
	RequestUpdate();
}

void UHLWFacilityPowerSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bWorldStarted && bUpdatePending)
	{
		UpdatePowerAvailability();
	}
	if (CVarPowerDebug.GetValueOnGameThread() > 0)
	{
		DrawDebug();
	}
}

TStatId UHLWFacilityPowerSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UHLWFacilityPowerSubsystem, STATGROUP_Tickables);
}

void UHLWFacilityPowerSubsystem::RegisterGenerator(AHLWPowerGenerator* Generator)
{
	if (!Generator)
	{
		return;
	}
	const FName ID = Generator->GetGeneratorID();
	if (const TWeakObjectPtr<AHLWPowerGenerator>* Existing = Generators.Find(ID); Existing && Existing->IsValid() && Existing->Get() != Generator)
	{
		UE_LOG(LogHLWPower, Warning, TEXT("Generator ID %s is used by %s and %s"), *ID.ToString(), *Existing->Get()->GetName(), *Generator->GetName());
	}
	Generators.Add(ID, Generator);
	RequestUpdate();
}

void UHLWFacilityPowerSubsystem::UnregisterGenerator(AHLWPowerGenerator* Generator)
{
	if (Generator)
	{
		Generators.Remove(Generator->GetGeneratorID());
		RequestUpdate();
	}
}

void UHLWFacilityPowerSubsystem::RegisterCircuit(const FHLWCircuitData& CircuitData)
{
	if (CircuitData.CircuitID.IsNone())
	{
		UE_LOG(LogHLWPower, Warning, TEXT("Circuit registered without an ID"));
		return;
	}
	if (Circuits.Contains(CircuitData.CircuitID))
	{
		UE_LOG(LogHLWPower, Warning, TEXT("Circuit ID %s registered twice, keeping the newest"), *CircuitData.CircuitID.ToString());
	}
	if (CircuitData.SourceID == CircuitData.CircuitID)
	{
		UE_LOG(LogHLWPower, Warning, TEXT("Circuit %s lists itself as its source"), *CircuitData.CircuitID.ToString());
	}

	FCircuitEntry& Entry = Circuits.FindOrAdd(CircuitData.CircuitID);
	Entry.Data = CircuitData;
	RequestUpdate();
}

void UHLWFacilityPowerSubsystem::UnregisterCircuit(FName CircuitID)
{
	if (Circuits.Remove(CircuitID) > 0)
	{
		RequestUpdate();
	}
}

void UHLWFacilityPowerSubsystem::RegisterConsumer(UHLWPowerConsumerComponent* Consumer)
{
	if (!Consumer)
	{
		return;
	}
	Consumers.AddUnique(Consumer);

	// Late arrivals get the current state silently. Early ones wait for the first resolve.
	if (bInitialResolveDone)
	{
		Consumer->ReceivePowerUpdate(GetCircuitState(Consumer->GetCircuitID()), true);
	}
	if (Consumer->GetConsumerData().Load > 0.f)
	{
		RequestUpdate();
	}
}

void UHLWFacilityPowerSubsystem::UnregisterConsumer(UHLWPowerConsumerComponent* Consumer)
{
	Consumers.Remove(Consumer);
}

void UHLWFacilityPowerSubsystem::SetCircuitEnabled(FName CircuitID, bool bEnabled)
{
	if (CircuitID.IsNone())
	{
		return;
	}
	EnabledOverrides.Add(CircuitID, bEnabled);
	if (UHLWPowerStateSubsystem* SaveState = UHLWPowerStateSubsystem::Get(this))
	{
		SaveState->SetCircuitEnabled(CircuitID, bEnabled);
	}
	RequestUpdate();
}

bool UHLWFacilityPowerSubsystem::IsCircuitEnabled(FName CircuitID) const
{
	if (const bool* Override = EnabledOverrides.Find(CircuitID))
	{
		return *Override;
	}
	const FCircuitEntry* Entry = Circuits.Find(CircuitID);
	return Entry && Entry->Data.bEnabledByDefault;
}

void UHLWFacilityPowerSubsystem::SetCircuitFaultSource(FName CircuitID, FName SourceID, bool bFaulted)
{
	if (CircuitID.IsNone())
	{
		return;
	}

	const bool bWasFaulted = IsCircuitFaulted(CircuitID);
	if (bFaulted)
	{
		FaultSources.FindOrAdd(CircuitID).Add(SourceID);
	}
	else if (TSet<FName>* Sources = FaultSources.Find(CircuitID))
	{
		Sources->Remove(SourceID);
		if (Sources->IsEmpty())
		{
			FaultSources.Remove(CircuitID);
		}
	}

	if (bWasFaulted != IsCircuitFaulted(CircuitID))
	{
		RequestUpdate();
	}
}

EHLWPowerState UHLWFacilityPowerSubsystem::GetCircuitState(FName CircuitID) const
{
	const FCircuitEntry* Entry = Circuits.Find(CircuitID);
	return Entry ? Entry->State : EHLWPowerState::Unpowered;
}

bool UHLWFacilityPowerSubsystem::HasIncomingPower(FName CircuitID) const
{
	const FCircuitEntry* Entry = Circuits.Find(CircuitID);
	if (!Entry)
	{
		return false;
	}

	auto SourceIsLive = [this](FName SourceID)
	{
		if (SourceID.IsNone())
		{
			return false;
		}
		if (const AHLWPowerGenerator* Generator = FindGenerator(SourceID))
		{
			return Generator->IsRunning();
		}
		return HLWPower::IsLive(GetCircuitState(SourceID));
	};

	return SourceIsLive(Entry->Data.SourceID) || SourceIsLive(Entry->Data.BackupSourceID);
}

bool UHLWFacilityPowerSubsystem::GetCircuitData(FName CircuitID, FHLWCircuitData& OutData) const
{
	if (const FCircuitEntry* Entry = Circuits.Find(CircuitID))
	{
		OutData = Entry->Data;
		return true;
	}
	return false;
}

FText UHLWFacilityPowerSubsystem::GetCircuitDisplayName(FName CircuitID) const
{
	const FCircuitEntry* Entry = Circuits.Find(CircuitID);
	return Entry && !Entry->Data.DisplayName.IsEmpty() ? Entry->Data.DisplayName : FText::FromName(CircuitID);
}

TArray<FName> UHLWFacilityPowerSubsystem::GetCircuitIDs() const
{
	TArray<FName> IDs;
	Circuits.GetKeys(IDs);
	return IDs;
}

TArray<FName> UHLWFacilityPowerSubsystem::GetGeneratorIDs() const
{
	TArray<FName> IDs;
	Generators.GetKeys(IDs);
	return IDs;
}

AHLWPowerGenerator* UHLWFacilityPowerSubsystem::FindGenerator(FName GeneratorID) const
{
	const TWeakObjectPtr<AHLWPowerGenerator>* Found = Generators.Find(GeneratorID);
	return Found ? Found->Get() : nullptr;
}

void UHLWFacilityPowerSubsystem::RestoreFacilityPower()
{
	for (const TPair<FName, FCircuitEntry>& Pair : Circuits)
	{
		SetCircuitEnabled(Pair.Key, true);
	}
	for (const TPair<FName, TWeakObjectPtr<AHLWPowerGenerator>>& Pair : Generators)
	{
		AHLWPowerGenerator* Generator = Pair.Value.Get();
		if (Generator && !Generator->GetGeneratorData().bIsBackup)
		{
			Generator->StartGenerator();
		}
	}
	RequestUpdate();
}

void UHLWFacilityPowerSubsystem::ActivateEmergencyPower()
{
	for (const TPair<FName, TWeakObjectPtr<AHLWPowerGenerator>>& Pair : Generators)
	{
		AHLWPowerGenerator* Generator = Pair.Value.Get();
		if (Generator && Generator->GetGeneratorData().bIsBackup)
		{
			Generator->StartGenerator();
		}
	}
}

void UHLWFacilityPowerSubsystem::NotifyElectricalFault(FName FaultID, FName TargetID, EHLWElectricalFaultType FaultType, bool bActive)
{
	OnElectricalFault.Broadcast(FaultID, TargetID, FaultType, bActive);
}

void UHLWFacilityPowerSubsystem::NotifyFuseCollected(FName FuseType, AActor* Collector)
{
	OnFuseCollected.Broadcast(FuseType, Collector);
}

void UHLWFacilityPowerSubsystem::NotifyGeneratorStateChanged(AHLWPowerGenerator* Generator)
{
	if (!Generator)
	{
		return;
	}
	OnGeneratorStateChanged.Broadcast(Generator->GetGeneratorID(), Generator->GetGeneratorState());
	RequestUpdate();
}

void UHLWFacilityPowerSubsystem::UpdatePowerAvailability()
{
	if (bResolving)
	{
		bUpdatePending = true;
		return;
	}
	TGuardValue<bool> ResolveGuard(bResolving, true);
	bUpdatePending = false;

	FResolveContext Context;
	for (int32 Pass = 0; Pass < MaxResolvePasses; ++Pass)
	{
		Context = FResolveContext();
		for (const TPair<FName, FCircuitEntry>& Pair : Circuits)
		{
			ResolveCircuit(Pair.Key, Context);
		}
		if (!FailOverloadedGenerators(Context))
		{
			break;
		}
	}

	const bool bInitial = !bInitialResolveDone || bNextResolveSilent;
	bInitialResolveDone = true;
	bNextResolveSilent = false;

	TArray<TPair<FName, EHLWPowerState>> Changed;
	for (TPair<FName, FCircuitEntry>& Pair : Circuits)
	{
		const EHLWPowerState NewState = Context.States.FindRef(Pair.Key);
		Pair.Value.FeedingGenerator = Context.Feeding.FindRef(Pair.Key);
		if (bInitial || NewState != Pair.Value.State)
		{
			Pair.Value.State = NewState;
			Changed.Emplace(Pair.Key, NewState);
		}
	}

	// Consumers first, so listeners see equipment already updated
	PushToConsumers(bInitial);
	for (const TPair<FName, EHLWPowerState>& Change : Changed)
	{
		UE_CLOG(!bInitial, LogHLWPower, Log, TEXT("Circuit %s -> %s"), *Change.Key.ToString(), *UEnum::GetValueAsString(Change.Value));
		OnCircuitStateChanged.Broadcast(Change.Key, Change.Value, bInitial);
	}
}

EHLWPowerState UHLWFacilityPowerSubsystem::ResolveCircuit(FName CircuitID, FResolveContext& Context) const
{
	if (const EHLWPowerState* Done = Context.States.Find(CircuitID))
	{
		return *Done;
	}

	const FCircuitEntry* Entry = Circuits.Find(CircuitID);
	if (!Entry)
	{
		return EHLWPowerState::Unpowered;
	}
	if (Context.Visiting.Contains(CircuitID))
	{
		UE_LOG(LogHLWPower, Warning, TEXT("Circuit loop through %s"), *CircuitID.ToString());
		return EHLWPowerState::Unpowered;
	}
	Context.Visiting.Add(CircuitID);

	EHLWPowerState State = EHLWPowerState::Unpowered;
	FName Generator;

	if (IsCircuitFaulted(CircuitID))
	{
		State = EHLWPowerState::Faulted;
	}
	else if (!IsCircuitEnabled(CircuitID))
	{
		State = EHLWPowerState::Off;
	}
	else
	{
		State = ResolveSupply(Entry->Data.SourceID, Context, Generator);
		if (State == EHLWPowerState::Unpowered && !Entry->Data.BackupSourceID.IsNone())
		{
			// Anything arriving through the backup path is backup power
			if (ResolveSupply(Entry->Data.BackupSourceID, Context, Generator) != EHLWPowerState::Unpowered)
			{
				State = EHLWPowerState::Backup;
			}
		}
	}

	Context.Visiting.Remove(CircuitID);
	Context.States.Add(CircuitID, State);
	if (HLWPower::IsLive(State))
	{
		Context.Feeding.Add(CircuitID, Generator);
	}
	return State;
}

EHLWPowerState UHLWFacilityPowerSubsystem::ResolveSupply(FName SourceID, FResolveContext& Context, FName& OutGenerator) const
{
	if (SourceID.IsNone())
	{
		return EHLWPowerState::Unpowered;
	}
	if (const AHLWPowerGenerator* Generator = FindGenerator(SourceID))
	{
		if (Generator->IsRunning())
		{
			OutGenerator = SourceID;
			return EHLWPowerState::Powered;
		}
		return EHLWPowerState::Unpowered;
	}
	if (Circuits.Contains(SourceID))
	{
		const EHLWPowerState Parent = ResolveCircuit(SourceID, Context);
		if (HLWPower::IsLive(Parent))
		{
			OutGenerator = Context.Feeding.FindRef(SourceID);
			return Parent;
		}
	}
	return EHLWPowerState::Unpowered;
}

bool UHLWFacilityPowerSubsystem::FailOverloadedGenerators(const FResolveContext& Context)
{
	TMap<FName, float> Loads;
	for (const TPair<FName, FName>& Feed : Context.Feeding)
	{
		if (const FCircuitEntry* Entry = Circuits.Find(Feed.Key))
		{
			Loads.FindOrAdd(Feed.Value) += Entry->Data.Load;
		}
	}
	for (const TWeakObjectPtr<UHLWPowerConsumerComponent>& Weak : Consumers)
	{
		const UHLWPowerConsumerComponent* Consumer = Weak.Get();
		if (!Consumer || Consumer->GetConsumerData().Load <= 0.f)
		{
			continue;
		}
		const FName CircuitID = Consumer->GetCircuitID();
		const EHLWPowerState State = Context.States.FindRef(CircuitID);
		if (HLWPower::SuppliesConsumer(State, Consumer->GetConsumerData().bEmergencyLoad))
		{
			if (const FName* Generator = Context.Feeding.Find(CircuitID))
			{
				Loads.FindOrAdd(*Generator) += Consumer->GetConsumerData().Load;
			}
		}
	}

	bool bAnyFailed = false;
	for (const TPair<FName, float>& Load : Loads)
	{
		AHLWPowerGenerator* Generator = FindGenerator(Load.Key);
		const float Capacity = Generator ? Generator->GetGeneratorData().Capacity : 0.f;
		if (Capacity > 0.f && Load.Value > Capacity)
		{
			UE_LOG(LogHLWPower, Log, TEXT("Generator %s overloaded (%.1f / %.1f)"), *Load.Key.ToString(), Load.Value, Capacity);
			Generator->TriggerGeneratorFailure();
			bAnyFailed = true;
		}
	}
	return bAnyFailed;
}

void UHLWFacilityPowerSubsystem::PushToConsumers(bool bInitial)
{
	Consumers.RemoveAll([](const TWeakObjectPtr<UHLWPowerConsumerComponent>& Weak) { return !Weak.IsValid(); });

	// Copy, a consumer may unregister while reacting
	const TArray<TWeakObjectPtr<UHLWPowerConsumerComponent>> Snapshot = Consumers;
	for (const TWeakObjectPtr<UHLWPowerConsumerComponent>& Weak : Snapshot)
	{
		if (UHLWPowerConsumerComponent* Consumer = Weak.Get())
		{
			Consumer->ReceivePowerUpdate(GetCircuitState(Consumer->GetCircuitID()), bInitial);
		}
	}
}

void UHLWFacilityPowerSubsystem::DrawDebug() const
{
	if (!GEngine)
	{
		return;
	}

	// Fixed keys so lines update in place
	uint64 Key = 0x48574C50;
	GEngine->AddOnScreenDebugMessage(Key++, 0.f, FColor::White, TEXT("--- HLW Power ---"));

	for (const TPair<FName, TWeakObjectPtr<AHLWPowerGenerator>>& Pair : Generators)
	{
		if (const AHLWPowerGenerator* Generator = Pair.Value.Get())
		{
			const bool bRunning = Generator->IsRunning();
			FString Line = FString::Printf(TEXT("GEN  %s  %s"), *Pair.Key.ToString(), *UEnum::GetDisplayValueAsText(Generator->GetGeneratorState()).ToString());
			if (Generator->GetGeneratorData().bRequiresFuel)
			{
				Line += FString::Printf(TEXT("  fuel %.0f"), Generator->GetFuel());
			}
			GEngine->AddOnScreenDebugMessage(Key++, 0.f, bRunning ? FColor::Green : FColor::Red, Line);
		}
	}

	TArray<FName> IDs = GetCircuitIDs();
	IDs.Sort(FNameLexicalLess());
	for (const FName& ID : IDs)
	{
		const FCircuitEntry& Entry = Circuits[ID];
		const FColor Color = Entry.State == EHLWPowerState::Powered ? FColor::Green
			: Entry.State == EHLWPowerState::Backup ? FColor::Yellow
			: Entry.State == EHLWPowerState::Faulted ? FColor::Red
			: FColor::Silver;

		const FString Line = FString::Printf(TEXT("CIR  %s  %s  src %s%s%s"),
			*ID.ToString(),
			*UEnum::GetDisplayValueAsText(Entry.State).ToString(),
			*Entry.Data.SourceID.ToString(),
			Entry.Data.BackupSourceID.IsNone() ? TEXT("") : *FString::Printf(TEXT("  backup %s"), *Entry.Data.BackupSourceID.ToString()),
			Entry.FeedingGenerator.IsNone() ? TEXT("") : *FString::Printf(TEXT("  via %s"), *Entry.FeedingGenerator.ToString()));
		GEngine->AddOnScreenDebugMessage(Key++, 0.f, Color, Line);
	}
}
