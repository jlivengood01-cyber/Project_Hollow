#include "Electrical/HLWPoweredLight.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Electrical/HLWPowerConsumerComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AHLWPoweredLight::AHLWPoweredLight()
{
	Fixture = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Fixture"));
	SetRootComponent(Fixture);

	// Placeholder fixture, replace in the Blueprint
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Fixture->SetStaticMesh(CubeMesh.Object);
		Fixture->SetRelativeScale3D(FVector(0.4f, 0.4f, 0.05f));
	}
	Fixture->SetCollisionProfileName(TEXT("NoCollision"));

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Fixture);
	Light->SetRelativeLocation(FVector(0.f, 0.f, -30.f));
	Light->SetAttenuationRadius(800.f);
}

void AHLWPoweredLight::BeginPlay()
{
	NormalColor = Light->GetLightColor();
	if (!EmissiveParameter.IsNone())
	{
		FixtureMaterial = Fixture->CreateDynamicMaterialInstance(0);
	}

	// Base applies the current power state
	Super::BeginPlay();
}

void AHLWPoweredLight::ApplyPower(bool bPowered, bool bInitial)
{
	GetWorldTimerManager().ClearTimer(FlickerTimer);

	if (bPowered && !bInitial && bFlickerOnRestore)
	{
		// Starts dark and ends on, so an odd step count is used
		FlickerStepsLeft = FlickerCount | 1;
		SetLightOn(false);
		FlickerStep();
	}
	else
	{
		SetLightOn(bPowered);
	}
	OnLightPowerChanged(bPowered, bInitial);
}

void AHLWPoweredLight::ApplyCircuitState(EHLWPowerState State, bool bInitial)
{
	const bool bBackup = bTintOnBackup && State == EHLWPowerState::Backup;
	Light->SetLightColor(bBackup ? BackupColor : NormalColor);
}

void AHLWPoweredLight::FlickerStep()
{
	if (FlickerStepsLeft <= 0)
	{
		SetLightOn(IsPowered());
		return;
	}
	--FlickerStepsLeft;
	SetLightOn(!Light->IsVisible());

	const float Delay = FMath::FRandRange(FlickerMinInterval, FMath::Max(FlickerMinInterval, FlickerMaxInterval));
	GetWorldTimerManager().SetTimer(FlickerTimer, this, &ThisClass::FlickerStep, Delay, false);
}

void AHLWPoweredLight::SetLightOn(bool bOn)
{
	Light->SetVisibility(bOn);
	if (FixtureMaterial)
	{
		FixtureMaterial->SetScalarParameterValue(EmissiveParameter, bOn ? EmissiveOn : EmissiveOff);
	}
}
