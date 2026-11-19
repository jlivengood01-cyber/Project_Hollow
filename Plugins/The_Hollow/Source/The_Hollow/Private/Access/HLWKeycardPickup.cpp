#include "Access/HLWKeycardPickup.h"

#include "Access/HLWAccessStateSubsystem.h"
#include "Access/HLWFacilityAccessSubsystem.h"
#include "Access/HLWKeycardDefinition.h"
#include "Access/HLWKeycardInventoryComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "HLWKeycardPickup"

AHLWKeycardPickup::AHLWKeycardPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	// Placeholder card shape, replace in the Blueprint
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
		Mesh->SetRelativeScale3D(FVector(0.2f, 0.12f, 0.02f));
	}
}

void AHLWKeycardPickup::BeginPlay()
{
	Super::BeginPlay();

	PickupID = HLWAccess::MakeStableID(this);
	if (UHLWAccessStateSubsystem* State = UHLWAccessStateSubsystem::Get(this))
	{
		State->OnStateRestored.AddDynamic(this, &ThisClass::HandleStateRestored);
		HandleStateRestored();
	}
}

void AHLWKeycardPickup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UHLWAccessStateSubsystem* State = UHLWAccessStateSubsystem::Get(this))
	{
		State->OnStateRestored.RemoveDynamic(this, &ThisClass::HandleStateRestored);
	}
	Super::EndPlay(EndPlayReason);
}

void AHLWKeycardPickup::HandleStateRestored()
{
	if (const UHLWAccessStateSubsystem* State = UHLWAccessStateSubsystem::Get(this))
	{
		SetCollectedState(State->IsPickupCollected(PickupID));
	}
}

void AHLWKeycardPickup::SetCollectedState(bool bNewCollected)
{
	// Hidden rather than destroyed so a load can bring it back
	bCollected = bNewCollected;
	SetActorHiddenInGame(bCollected);
	SetActorEnableCollision(!bCollected);
}

bool AHLWKeycardPickup::CollectKeycard(AActor* Collector)
{
	if (bCollected || !Keycard || !Collector)
	{
		return false;
	}

	UHLWKeycardInventoryComponent* Inventory = Collector->FindComponentByClass<UHLWKeycardInventoryComponent>();
	if (!Inventory)
	{
		UE_LOG(LogHLWAccess, Warning, TEXT("%s has no keycard inventory"), *Collector->GetName());
		return false;
	}
	if (!Inventory->AddKeycard(Keycard))
	{
		return false;
	}

	SetCollectedState(true);
	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
	}

	if (UHLWAccessStateSubsystem* State = UHLWAccessStateSubsystem::Get(this))
	{
		State->SetPickupCollected(PickupID);
	}

	OnCollected.Broadcast(Keycard, Collector);
	if (UHLWFacilityAccessSubsystem* Access = GetWorld()->GetSubsystem<UHLWFacilityAccessSubsystem>())
	{
		Access->OnKeycardCollected.Broadcast(Keycard, Collector);
	}
	return true;
}

void AHLWKeycardPickup::Interact_Implementation(AActor* InteractingActor)
{
	CollectKeycard(InteractingActor);
}

FText AHLWKeycardPickup::GetInteractionText_Implementation()
{
	return Keycard
		? FText::Format(LOCTEXT("PickUp", "Pick up {0}"), Keycard->DisplayName)
		: FText::GetEmpty();
}

bool AHLWKeycardPickup::CanInteract_Implementation()
{
	return !bCollected && Keycard != nullptr;
}

#undef LOCTEXT_NAMESPACE
