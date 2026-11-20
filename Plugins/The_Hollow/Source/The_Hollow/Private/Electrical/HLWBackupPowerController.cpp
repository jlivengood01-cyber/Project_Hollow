#include "Electrical/HLWBackupPowerController.h"

#include "Components/BillboardComponent.h"
#include "Electrical/HLWFacilityPowerSubsystem.h"
#include "Electrical/HLWPowerGenerator.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

AHLWBackupPowerController::AHLWBackupPowerController()
{
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

#if WITH_EDITORONLY_DATA
	Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (Sprite)
	{
		Sprite->SetupAttachment(GetRootComponent());
	}
#endif
}

void AHLWBackupPowerController::BeginPlay()
{
	Super::BeginPlay();

	if (UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>())
	{
		Power->OnGeneratorStateChanged.AddDynamic(this, &ThisClass::HandleGeneratorStateChanged);
	}

	// Catches a level that starts with main power already down
	Evaluate();
}

void AHLWBackupPowerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	if (UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>())
	{
		Power->OnGeneratorStateChanged.RemoveDynamic(this, &ThisClass::HandleGeneratorStateChanged);
	}
	Super::EndPlay(EndPlayReason);
}

void AHLWBackupPowerController::HandleGeneratorStateChanged(FName GeneratorID, EHLWGeneratorState State)
{
	if (IsMainGenerator(GeneratorID))
	{
		Evaluate();
	}
}

bool AHLWBackupPowerController::IsMainPowerDown() const
{
	const UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>();
	if (!Power)
	{
		return false;
	}

	bool bAnyMain = false;
	bool bAnyBroken = false;
	for (const FName& ID : GetMainIDs())
	{
		const AHLWPowerGenerator* Generator = Power->FindGenerator(ID);
		if (!Generator)
		{
			continue;
		}
		bAnyMain = true;

		const EHLWGeneratorState State = Generator->GetGeneratorState();
		if (State == EHLWGeneratorState::Running || State == EHLWGeneratorState::Starting)
		{
			return false;
		}
		bAnyBroken |= State == EHLWGeneratorState::Failed || State == EHLWGeneratorState::NoFuel;
	}
	return bAnyMain && (bAnyBroken || bStartOnManualStop);
}

void AHLWBackupPowerController::Evaluate()
{
	FTimerManager& Timers = GetWorldTimerManager();

	if (IsMainPowerDown())
	{
		if (!Timers.IsTimerActive(StartTimer))
		{
			Report(FString::Printf(TEXT("Main power lost, backup in %.0fs"), StartDelay));
			Timers.SetTimer(StartTimer, this, &ThisClass::StartBackup, FMath::Max(StartDelay, 0.01f), false);
		}
		return;
	}

	Timers.ClearTimer(StartTimer);
	if (bStopWhenMainReturns)
	{
		StopBackup();
	}
}

void AHLWBackupPowerController::StartBackup()
{
	// Main may have come back during the delay
	if (!IsMainPowerDown())
	{
		return;
	}

	const UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>();
	for (const FName& ID : GetBackupIDs())
	{
		if (AHLWPowerGenerator* Generator = Power ? Power->FindGenerator(ID) : nullptr)
		{
			Generator->StartGenerator();
		}
	}
	Report(TEXT("Backup power started"));
}

void AHLWBackupPowerController::StopBackup()
{
	const UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>();
	bool bStopped = false;
	for (const FName& ID : GetBackupIDs())
	{
		AHLWPowerGenerator* Generator = Power ? Power->FindGenerator(ID) : nullptr;
		if (Generator && (Generator->IsRunning() || Generator->GetGeneratorState() == EHLWGeneratorState::Starting))
		{
			Generator->StopGenerator();
			bStopped = true;
		}
	}
	if (bStopped)
	{
		Report(TEXT("Main power back, backup stopped"));
	}
}

bool AHLWBackupPowerController::IsMainGenerator(FName GeneratorID) const
{
	return GetMainIDs().Contains(GeneratorID);
}

TArray<FName> AHLWBackupPowerController::GetMainIDs() const
{
	if (!MainGeneratorIDs.IsEmpty())
	{
		return MainGeneratorIDs;
	}

	TArray<FName> IDs;
	if (const UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>())
	{
		for (const FName& ID : Power->GetGeneratorIDs())
		{
			const AHLWPowerGenerator* Generator = Power->FindGenerator(ID);
			if (Generator && !Generator->GetGeneratorData().bIsBackup)
			{
				IDs.Add(ID);
			}
		}
	}
	return IDs;
}

TArray<FName> AHLWBackupPowerController::GetBackupIDs() const
{
	if (!BackupGeneratorIDs.IsEmpty())
	{
		return BackupGeneratorIDs;
	}

	TArray<FName> IDs;
	if (const UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>())
	{
		for (const FName& ID : Power->GetGeneratorIDs())
		{
			const AHLWPowerGenerator* Generator = Power->FindGenerator(ID);
			if (Generator && Generator->GetGeneratorData().bIsBackup)
			{
				IDs.Add(ID);
			}
		}
	}
	return IDs;
}

void AHLWBackupPowerController::Report(const FString& Message) const
{
	UE_LOG(LogHLWPower, Log, TEXT("%s"), *Message);
	if (bShowDebugResult && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, Message);
	}
}
