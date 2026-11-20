#include "Electrical/HLWElectricalHazard.h"

#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AHLWElectricalHazardTrigger::AHLWElectricalHazardTrigger()
{
	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->SetBoxExtent(FVector(60.f, 60.f, 60.f));
	Trigger->SetCollisionProfileName(TEXT("Trigger"));

	Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	Audio->SetupAttachment(Trigger);
	Audio->bAutoActivate = false;
}

void AHLWElectricalHazardTrigger::BeginPlay()
{
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleBeginOverlap);
	Super::BeginPlay();
}

void AHLWElectricalHazardTrigger::ApplyPower(bool bPowered, bool bInitial)
{
	if (bPowered && LiveLoopSound)
	{
		Audio->SetSound(LiveLoopSound);
		Audio->Play();
	}
	else
	{
		Audio->Stop();
	}

	// Anyone already standing in it gets caught when it goes live
	if (bPowered && !bInitial)
	{
		TArray<AActor*> Overlapping;
		Trigger->GetOverlappingActors(Overlapping, APawn::StaticClass());
		for (AActor* Actor : Overlapping)
		{
			Touch(Actor);
		}
	}
	OnHazardLiveChanged(bPowered, bInitial);
}

void AHLWElectricalHazardTrigger::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (IsLive() && Cast<APawn>(OtherActor))
	{
		Touch(OtherActor);
	}
}

void AHLWElectricalHazardTrigger::Touch(AActor* Victim)
{
	OnHazardTouched.Broadcast(Victim);
	UE_LOG(LogHLWPower, Log, TEXT("%s touched live hazard %s"), *Victim->GetName(), *GetName());
	if (bShowDebugResult && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, FString::Printf(TEXT("Shock from %s"), *GetName()));
	}
}

AHLWExposedWiring::AHLWExposedWiring()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Trigger);
	Mesh->SetCollisionProfileName(TEXT("NoCollision"));

	// Placeholder cable, replace in the Blueprint
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CylinderMesh.Object);
		Mesh->SetRelativeScale3D(FVector(0.05f, 0.05f, 1.f));
		Mesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	}
}

AHLWSparkingPanel::AHLWSparkingPanel()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Trigger);

	// Placeholder panel, replace in the Blueprint
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
		Mesh->SetRelativeScale3D(FVector(0.1f, 0.6f, 0.8f));
	}

	SparkLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("SparkLight"));
	SparkLight->SetupAttachment(Trigger);
	SparkLight->SetRelativeLocation(FVector(20.f, 0.f, 0.f));
	SparkLight->SetAttenuationRadius(300.f);
	SparkLight->SetLightColor(FLinearColor(0.6f, 0.8f, 1.f));
	SparkLight->SetCastShadows(false);
	SparkLight->SetIntensity(0.f);
}

void AHLWSparkingPanel::ApplyPower(bool bPowered, bool bInitial)
{
	Super::ApplyPower(bPowered, bInitial);

	GetWorldTimerManager().ClearTimer(SparkTimer);
	SparkLight->SetIntensity(0.f);
	if (bPowered)
	{
		Spark();
	}
}

void AHLWSparkingPanel::Spark()
{
	const bool bFlash = FMath::FRand() < 0.4f;
	SparkLight->SetIntensity(bFlash ? SparkIntensity * FMath::FRandRange(0.5f, 1.f) : 0.f);
	GetWorldTimerManager().SetTimer(SparkTimer, this, &ThisClass::Spark, FMath::FRandRange(0.03f, 0.4f), false);
}
