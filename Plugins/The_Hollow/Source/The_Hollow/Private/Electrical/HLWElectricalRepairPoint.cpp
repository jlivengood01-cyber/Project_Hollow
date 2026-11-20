#include "Electrical/HLWElectricalRepairPoint.h"

#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Electrical/HLWElectricalFaultComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AHLWElectricalRepairPoint::AHLWElectricalRepairPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	// Placeholder wall panel, replace in the Blueprint
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
		Mesh->SetRelativeScale3D(FVector(0.1f, 0.5f, 0.5f));
	}

	Fault = CreateDefaultSubobject<UHLWElectricalFaultComponent>(TEXT("Fault"));

	SparkLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("SparkLight"));
	SparkLight->SetupAttachment(Mesh);
	SparkLight->SetRelativeLocation(FVector(80.f, 0.f, 0.f));
	SparkLight->SetAttenuationRadius(250.f);
	SparkLight->SetLightColor(FLinearColor(0.6f, 0.8f, 1.f));
	SparkLight->SetCastShadows(false);
	SparkLight->SetIntensity(0.f);

	Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	Audio->SetupAttachment(Mesh);
	Audio->bAutoActivate = false;
}

void AHLWElectricalRepairPoint::BeginPlay()
{
	Super::BeginPlay();

	Fault->OnFaultChanged.AddDynamic(this, &ThisClass::HandleFaultChanged);
	HandleFaultChanged(Fault->IsFaultActive());
}

void AHLWElectricalRepairPoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	Super::EndPlay(EndPlayReason);
}

void AHLWElectricalRepairPoint::HandleFaultChanged(bool bActive)
{
	GetWorldTimerManager().ClearTimer(SparkTimer);
	SparkLight->SetIntensity(0.f);

	if (bActive)
	{
		Spark();
		if (DamagedLoopSound)
		{
			Audio->SetSound(DamagedLoopSound);
			Audio->Play();
		}
	}
	else
	{
		Audio->Stop();
	}
	OnFaultStateChanged(bActive);
}

void AHLWElectricalRepairPoint::Spark()
{
	const bool bFlash = FMath::FRand() < 0.4f;
	SparkLight->SetIntensity(bFlash ? SparkIntensity * FMath::FRandRange(0.5f, 1.f) : 0.f);
	GetWorldTimerManager().SetTimer(SparkTimer, this, &ThisClass::Spark, FMath::FRandRange(0.03f, 0.4f), false);
}

void AHLWElectricalRepairPoint::Interact_Implementation(AActor* InteractingActor)
{
	Fault->TryRepair(InteractingActor);
}

FText AHLWElectricalRepairPoint::GetInteractionText_Implementation()
{
	return Fault->IsFaultActive() ? Fault->GetRepairText() : FText::GetEmpty();
}

bool AHLWElectricalRepairPoint::CanInteract_Implementation()
{
	return Fault->IsFaultActive() && Fault->CanRepair(UGameplayStatics::GetPlayerPawn(this, 0));
}
