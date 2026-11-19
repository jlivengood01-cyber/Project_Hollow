// Fill out your copyright notice in the Description page of Project Settings.


#include "HLW_Security_System_Manager.h"

// Sets default values
AHLW_Security_System_Manager::AHLW_Security_System_Manager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void AHLW_Security_System_Manager::ReportPlayerSpotted(FVector PlayerLocation, UPrimaryDataAsset* Zone, AActor* SourceSensor)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();
	if (CurrentTime - LastAlertTime < AlertCooldown)
	{
		return;
	}

	LastAlertTime = CurrentTime;
	OnSecurityAlert.Broadcast(PlayerLocation, Zone);
}

// Called when the game starts or when spawned
void AHLW_Security_System_Manager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AHLW_Security_System_Manager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

