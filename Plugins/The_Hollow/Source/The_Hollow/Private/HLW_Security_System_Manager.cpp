// Fill out your copyright notice in the Description page of Project Settings.


#include "HLW_Security_System_Manager.h"

// Sets default values
AHLW_Security_System_Manager::AHLW_Security_System_Manager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

void AHLW_Security_System_Manager::ReportPlayerSpotted(FVector PlayerLocation, UPrimaryDataAsset* Zone, AActor* SourceSensor)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!bIsSystemActive) return;

	const float CurrentTime = World->GetTimeSeconds();
	if (CurrentTime - LastAlertTime < AlertCooldown)
	{
		return;
	}

	LastAlertTime = CurrentTime;

	OnSecurityAlert.Broadcast(PlayerLocation, Zone);
}

void AHLW_Security_System_Manager::DisableSecuritySystem(float OverrideDuration)
{
	const float Duration = (OverrideDuration > 0.0f) ? OverrideDuration : DefaultDisableDuration;

	// Reset/start the reactivation timer even if already disabled (extends the disable window)
	GetWorldTimerManager().SetTimer(
		ReactivateTimerHandle,
		this,
		&AHLW_Security_System_Manager::ReactivateSecuritySystem,
		Duration,
		false
	);

	// Only broadcast if the state actually changed
	if (bIsSystemActive)
	{
		bIsSystemActive = false;
		OnSecuritySystemStateChanged.Broadcast(false);
	}
}

void AHLW_Security_System_Manager::EnableSecuritySystem()
{
	GetWorldTimerManager().ClearTimer(ReactivateTimerHandle);

	if (!bIsSystemActive)
	{
		bIsSystemActive = true;
		OnSecuritySystemStateChanged.Broadcast(true);
	}
}

// Called when the game starts or when spawned
void AHLW_Security_System_Manager::BeginPlay()
{
	Super::BeginPlay();
	
}

void AHLW_Security_System_Manager::ReactivateSecuritySystem()
{
	EnableSecuritySystem();
}

// Called every frame
void AHLW_Security_System_Manager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

