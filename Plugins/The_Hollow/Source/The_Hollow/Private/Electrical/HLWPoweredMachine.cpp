#include "Electrical/HLWPoweredMachine.h"

#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "HLWPoweredMachine"

AHLWPoweredMachine::AHLWPoweredMachine()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	// Placeholder cabinet, replace in the Blueprint
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
		Mesh->SetRelativeScale3D(FVector(0.8f, 0.6f, 1.4f));
	}

	Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	Audio->SetupAttachment(Mesh);
	Audio->bAutoActivate = false;
}

void AHLWPoweredMachine::ApplyPower(bool bPowered, bool bInitial)
{
	if (bPowered && RunningSound)
	{
		Audio->SetSound(RunningSound);
		Audio->Play();
	}
	else
	{
		Audio->Stop();
	}

	USoundBase* OneShot = bPowered ? StartSound.Get() : StopSound.Get();
	if (!bInitial && OneShot)
	{
		UGameplayStatics::PlaySoundAtLocation(this, OneShot, GetActorLocation());
	}
	OnMachinePowerChanged(bPowered, bInitial);
}

void AHLWPoweredMachine::Interact_Implementation(AActor* InteractingActor)
{
	const bool bPowered = IsPowered();
	OnInspected.Broadcast(bPowered);

	if (bShowDebugResult && GEngine)
	{
		const FString Message = FString::Printf(TEXT("%s: %s"), *GetMachineName().ToString(), bPowered ? TEXT("running") : TEXT("no power"));
		GEngine->AddOnScreenDebugMessage(-1, 3.f, bPowered ? FColor::Green : FColor::Red, Message);
	}
}

FText AHLWPoweredMachine::GetInteractionText_Implementation()
{
	return bCanInspect ? FText::Format(LOCTEXT("Inspect", "Inspect {0}"), GetMachineName()) : FText::GetEmpty();
}

bool AHLWPoweredMachine::CanInteract_Implementation()
{
	return bCanInspect;
}

FText AHLWPoweredMachine::GetMachineName() const
{
	return DisplayName.IsEmpty() ? LOCTEXT("Machine", "equipment") : DisplayName;
}

AHLWPoweredVentilation::AHLWPoweredVentilation()
{
	if (Mesh->GetStaticMesh())
	{
		Mesh->SetRelativeScale3D(FVector(1.f, 1.f, 0.3f));
	}

	Fan = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Fan"));
	Fan->SetupAttachment(Mesh);
	Fan->SetCollisionProfileName(TEXT("NoCollision"));

	// Placeholder blades, replace in the Blueprint
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BladeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (BladeMesh.Succeeded())
	{
		Fan->SetStaticMesh(BladeMesh.Object);
		Fan->SetRelativeScale3D(FVector(0.9f, 0.1f, 0.3f));
		Fan->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	}

	FanSpin = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("FanSpin"));
	FanSpin->SetUpdatedComponent(Fan);
	FanSpin->RotationRate = FRotator(0.f, 360.f, 0.f);
	FanSpin->bAutoActivate = false;

	DisplayName = LOCTEXT("Vent", "ventilation");
}

void AHLWPoweredVentilation::ApplyPower(bool bPowered, bool bInitial)
{
	Super::ApplyPower(bPowered, bInitial);
	FanSpin->SetActive(bPowered);
}

#undef LOCTEXT_NAMESPACE
