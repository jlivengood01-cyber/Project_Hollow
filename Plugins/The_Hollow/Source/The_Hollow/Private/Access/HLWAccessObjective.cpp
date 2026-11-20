#include "Access/HLWAccessObjective.h"

#include "Access/HLWAccessStateSubsystem.h"
#include "Access/HLWFacilityAccessSubsystem.h"
#include "Access/HLWKeycardDefinition.h"
#include "Access/HLWKeycardReader.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "HLWAccessObjective"

AHLWAccessObjective::AHLWAccessObjective()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	ObjectiveName = LOCTEXT("DefaultName", "Obtain Research Wing Access");
	FindCredentialText = LOCTEXT("DefaultFind", "Find a keycard with the right clearance.");
	ReturnToDeviceText = LOCTEXT("DefaultReturn", "Return to the locked entrance.");
	CompleteText = LOCTEXT("DefaultComplete", "Access obtained.");
}

void AHLWAccessObjective::BeginPlay()
{
	Super::BeginPlay();

	ObjectiveID = HLWAccess::MakeStableID(this);

	if (UHLWFacilityAccessSubsystem* Access = GetWorld()->GetSubsystem<UHLWFacilityAccessSubsystem>())
	{
		Access->OnAccessResolved.AddDynamic(this, &ThisClass::HandleAccessResolved);
		Access->OnKeycardCollected.AddDynamic(this, &ThisClass::HandleKeycardCollected);
	}
	if (UHLWAccessStateSubsystem* State = UHLWAccessStateSubsystem::Get(this))
	{
		State->OnStateRestored.AddDynamic(this, &ThisClass::HandleStateRestored);
		HandleStateRestored();
	}

	if (bStartActive && Stage == EHLWAccessObjectiveStage::Inactive)
	{
		SetStage(EHLWAccessObjectiveStage::FindCredential, true);
	}
}

void AHLWAccessObjective::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UHLWFacilityAccessSubsystem* Access = GetWorld()->GetSubsystem<UHLWFacilityAccessSubsystem>())
	{
		Access->OnAccessResolved.RemoveDynamic(this, &ThisClass::HandleAccessResolved);
		Access->OnKeycardCollected.RemoveDynamic(this, &ThisClass::HandleKeycardCollected);
	}
	if (UHLWAccessStateSubsystem* State = UHLWAccessStateSubsystem::Get(this))
	{
		State->OnStateRestored.RemoveDynamic(this, &ThisClass::HandleStateRestored);
	}
	Super::EndPlay(EndPlayReason);
}

FText AHLWAccessObjective::GetStageText() const
{
	switch (Stage)
	{
	case EHLWAccessObjectiveStage::FindCredential: return FindCredentialText;
	case EHLWAccessObjectiveStage::ReturnToDevice: return ReturnToDeviceText;
	case EHLWAccessObjectiveStage::Complete:       return CompleteText;
	default:                                       return FText::GetEmpty();
	}
}

void AHLWAccessObjective::HandleAccessResolved(FName DeviceID, EHLWAccessResult Result)
{
	// Read the ID at event time, the reader fills its default on BeginPlay
	if (!TargetReader || DeviceID != TargetReader->GetAccessData().DeviceID || Stage == EHLWAccessObjectiveStage::Complete)
	{
		return;
	}

	if (Result == EHLWAccessResult::Granted)
	{
		SetStage(EHLWAccessObjectiveStage::Complete, true);
	}
	else if (Stage == EHLWAccessObjectiveStage::Inactive)
	{
		SetStage(EHLWAccessObjectiveStage::FindCredential, true);
	}
}

void AHLWAccessObjective::HandleKeycardCollected(UHLWKeycardDefinition* Keycard, AActor* Collector)
{
	const bool bIsRequiredCard = Keycard && RequiredKeycard && Keycard->KeycardID == RequiredKeycard->KeycardID;
	if (bIsRequiredCard && Stage == EHLWAccessObjectiveStage::FindCredential)
	{
		SetStage(EHLWAccessObjectiveStage::ReturnToDevice, true);
	}
}

void AHLWAccessObjective::HandleStateRestored()
{
	if (const UHLWAccessStateSubsystem* State = UHLWAccessStateSubsystem::Get(this))
	{
		SetStage(static_cast<EHLWAccessObjectiveStage>(State->GetObjectiveStage(ObjectiveID)), false);
	}
}

void AHLWAccessObjective::SetStage(EHLWAccessObjectiveStage NewStage, bool bAnnounce)
{
	Stage = NewStage;

	if (UHLWAccessStateSubsystem* State = UHLWAccessStateSubsystem::Get(this))
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
