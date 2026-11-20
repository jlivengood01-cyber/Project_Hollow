#include "Electrical/HLWFusePickup.h"

#include "Access/HLWAccessTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Electrical/HLWFacilityPowerSubsystem.h"
#include "Electrical/HLWFuseDefinition.h"
#include "Electrical/HLWItemHolderComponent.h"
#include "Electrical/HLWPowerStateSubsystem.h"
#include "Electrical/HLWPowerTypes.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "HLWFusePickup"

AHLWFusePickup::AHLWFusePickup()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	// Placeholder fuse shape, replace in the Blueprint
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CylinderMesh.Object);
		Mesh->SetRelativeScale3D(FVector(0.08f, 0.08f, 0.2f));
	}
}

void AHLWFusePickup::BeginPlay()
{
	Super::BeginPlay();

	PickupID = HLWAccess::MakeStableID(this);
	if (UHLWPowerStateSubsystem* State = UHLWPowerStateSubsystem::Get(this))
	{
		State->OnStateRestored.AddDynamic(this, &ThisClass::HandleStateRestored);
		HandleStateRestored();
	}
}

void AHLWFusePickup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UHLWPowerStateSubsystem* State = UHLWPowerStateSubsystem::Get(this))
	{
		State->OnStateRestored.RemoveDynamic(this, &ThisClass::HandleStateRestored);
	}
	Super::EndPlay(EndPlayReason);
}

void AHLWFusePickup::HandleStateRestored()
{
	if (const UHLWPowerStateSubsystem* State = UHLWPowerStateSubsystem::Get(this))
	{
		SetCollectedState(State->IsPickupCollected(PickupID));
	}
}

void AHLWFusePickup::SetCollectedState(bool bNewCollected)
{
	bCollected = bNewCollected;
	SetActorHiddenInGame(bCollected);
	SetActorEnableCollision(!bCollected);
}

bool AHLWFusePickup::CollectFuse(AActor* Collector)
{
	if (bCollected || !Fuse || !Collector)
	{
		return false;
	}

	UHLWItemHolderComponent* Holder = Collector->FindComponentByClass<UHLWItemHolderComponent>();
	if (!Holder)
	{
		UE_LOG(LogHLWPower, Warning, TEXT("%s has no item holder"), *Collector->GetName());
		return false;
	}

	Holder->AddItem(Fuse->FuseType);
	SetCollectedState(true);
	if (UHLWPowerStateSubsystem* State = UHLWPowerStateSubsystem::Get(this))
	{
		State->SetPickupCollected(PickupID);
	}
	if (UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>())
	{
		Power->NotifyFuseCollected(Fuse->FuseType, Collector);
	}

	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, FString::Printf(TEXT("Picked up %s"), *Fuse->FuseType.ToString()));
	}
	return true;
}

void AHLWFusePickup::Interact_Implementation(AActor* InteractingActor)
{
	CollectFuse(InteractingActor);
}

FText AHLWFusePickup::GetInteractionText_Implementation()
{
	if (!Fuse)
	{
		return FText::GetEmpty();
	}
	const FText Name = Fuse->DisplayName.IsEmpty() ? LOCTEXT("Fuse", "fuse") : Fuse->DisplayName;
	return FText::Format(LOCTEXT("PickUp", "Pick up {0}"), Name);
}

bool AHLWFusePickup::CanInteract_Implementation()
{
	return !bCollected && Fuse != nullptr;
}

#undef LOCTEXT_NAMESPACE
