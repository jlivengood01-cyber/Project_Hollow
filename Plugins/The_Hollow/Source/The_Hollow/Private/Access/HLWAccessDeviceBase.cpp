#include "Access/HLWAccessDeviceBase.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AHLWAccessDeviceBase::AHLWAccessDeviceBase()
{
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetRootComponent());

	// Placeholder box, subclasses set the scale and Blueprints replace the mesh
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}
}

void AHLWAccessDeviceBase::SetPowered(bool bNewPowered)
{
	if (bPowered != bNewPowered)
	{
		bPowered = bNewPowered;
		OnDeviceStateChanged(true);
	}
}

void AHLWAccessDeviceBase::SetNetworked(bool bNewNetworked)
{
	if (bNetworked != bNewNetworked)
	{
		bNetworked = bNewNetworked;
		OnDeviceStateChanged(false);
	}
}

void AHLWAccessDeviceBase::SetOperational(bool bNewOperational)
{
	if (bOperational != bNewOperational)
	{
		bOperational = bNewOperational;
		OnDeviceStateChanged(false);
	}
}
