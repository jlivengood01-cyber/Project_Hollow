#include "Access/HLWAccessControlledDoor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "HLWAccessControlledDoor"

AHLWAccessControlledDoor::AHLWAccessControlledDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // Ticks only while moving

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(GetRootComponent());

	// Placeholder panel with its pivot at floor level, replace in the Blueprint
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		DoorMesh->SetStaticMesh(CubeMesh.Object);
		DoorMesh->SetRelativeScale3D(FVector(0.2f, 1.2f, 2.2f));
		DoorMesh->SetRelativeLocation(FVector(0.f, 0.f, 110.f));
	}
}

void AHLWAccessControlledDoor::BeginPlay()
{
	Super::BeginPlay();
	ClosedLocation = DoorMesh->GetRelativeLocation();
}

void AHLWAccessControlledDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// The motor stops mid-way if power drops
	if (!CanMove())
	{
		SetActorTickEnabled(false);
		return;
	}

	const float Target = bIsOpen ? 1.f : 0.f;
	OpenAlpha = FMath::FInterpConstantTo(OpenAlpha, Target, DeltaSeconds, 1.f / OpenTime);
	DoorMesh->SetRelativeLocation(ClosedLocation + OpenOffset * OpenAlpha);
	UpdateTick();
}

bool AHLWAccessControlledDoor::IsUnlocked() const
{
	const bool bPowerReleased = !bPowered && PowerFailureBehavior == EHLWPowerFailureBehavior::Unlock;
	return !bRequiresAccess || bUnlocked || bPowerReleased;
}

bool AHLWAccessControlledDoor::CanMove() const
{
	return bPowered || PowerFailureBehavior != EHLWPowerFailureBehavior::StayLocked;
}

void AHLWAccessControlledDoor::SetDoorOpen(bool bOpen)
{
	if (bIsOpen == bOpen)
	{
		return;
	}

	bIsOpen = bOpen;
	if (CanMove())
	{
		PlaySound(bIsOpen ? OpenSound : CloseSound);
	}
	UpdateTick();
}

void AHLWAccessControlledDoor::SetPowered(bool bNewPowered)
{
	if (bPowered == bNewPowered || PowerFailureBehavior == EHLWPowerFailureBehavior::KeepState)
	{
		bPowered = bNewPowered;
		return;
	}

	bPowered = bNewPowered;
	if (!bPowered)
	{
		PlaySound(PowerLostSound);
	}
	UpdateTick(); // Resumes any interrupted movement
}

void AHLWAccessControlledDoor::SetSecurityLocked(bool bLocked)
{
	bSecurityLocked = bLocked;
	if (bSecurityLocked && bCloseOnLockdown)
	{
		SetDoorOpen(false);
	}
}

void AHLWAccessControlledDoor::OnAccessGranted_Implementation(AActor* Reader)
{
	if (!bUnlocked)
	{
		bUnlocked = true;
		PlaySound(UnlockSound);
	}
	if (bOpenOnGrant && !bSecurityLocked && CanMove())
	{
		SetDoorOpen(true);
	}
}

void AHLWAccessControlledDoor::OnAccessDenied_Implementation(AActor* Reader, EHLWAccessResult Result)
{
	// Stays as it is; Blueprints can react here
}

void AHLWAccessControlledDoor::OnReaderPowerChanged_Implementation(AActor* Reader, bool bReaderPowered)
{
	SetPowered(bReaderPowered);
}

void AHLWAccessControlledDoor::OnSecurityLockChanged_Implementation(AActor* Reader, bool bLocked)
{
	SetSecurityLocked(bLocked);
}

void AHLWAccessControlledDoor::OnAccessStateRestored_Implementation(AActor* Reader, bool bRestoredUnlocked)
{
	bUnlocked = bRestoredUnlocked;
	if (!IsUnlocked())
	{
		SetDoorOpen(false);
	}
}

void AHLWAccessControlledDoor::Interact_Implementation(AActor* InteractingActor)
{
	if (CanInteract_Implementation())
	{
		SetDoorOpen(!bIsOpen);
	}
}

FText AHLWAccessControlledDoor::GetInteractionText_Implementation()
{
	if (bSecurityLocked)
	{
		return LOCTEXT("Lockdown", "Lockdown");
	}
	if (!CanMove())
	{
		return LOCTEXT("NoPower", "No power");
	}
	if (bIsOpen)
	{
		return LOCTEXT("Close", "Close door");
	}
	return IsUnlocked() ? LOCTEXT("Open", "Open door") : LOCTEXT("Locked", "Locked");
}

bool AHLWAccessControlledDoor::CanInteract_Implementation()
{
	// An open door can always be closed, even if it's locked
	return !bSecurityLocked && CanMove() && (bIsOpen || IsUnlocked());
}

void AHLWAccessControlledDoor::UpdateTick()
{
	const float Target = bIsOpen ? 1.f : 0.f;
	SetActorTickEnabled(CanMove() && !FMath::IsNearlyEqual(OpenAlpha, Target));
}

void AHLWAccessControlledDoor::PlaySound(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, DoorMesh->GetComponentLocation());
	}
}

#undef LOCTEXT_NAMESPACE
