// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataAsset.h"
#include "HLW_Security_System_Manager.generated.h"

//Broadcast to other AI Enemies
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnSecurityAlertSignature,
	FVector, AlertLocation,
	UPrimaryDataAsset*, AlertZone
);

//Broadcasts when the security system is enabled or disabled
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnSecuritySystemStateChangedSignature,
	bool, bIsActive
);

UCLASS(Blueprintable)
class THE_HOLLOW_API AHLW_Security_System_Manager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHLW_Security_System_Manager();

	UPROPERTY(BlueprintAssignable, Category = "Security")
	FOnSecurityAlertSignature OnSecurityAlert;

	// Optional: Cameras/Doors/UI can bind to this to show powered-off vs. active state
	//Doors/Cameras
	UPROPERTY(BlueprintAssignable, Category = "Security")
	FOnSecuritySystemStateChangedSignature OnSecuritySystemStateChanged;

	UFUNCTION(BlueprintPure, Category = "Security")
	bool IsSecuritySystemActive() const { return bIsSystemActive; }

	// Function called by Cameras / Watcher AI
	UFUNCTION(BlueprintCallable, Category = "Security")
	void ReportPlayerSpotted(FVector PlayerLocation, UPrimaryDataAsset* Zone, AActor* SourceSensor = nullptr);

	// Called by Terminals / Switches to temporarily or permanently disable the system
	UFUNCTION(BlueprintCallable, Category = "Security")
	void DisableSecuritySystem(float OverrideDuration = -1.0f);

	UFUNCTION(BlueprintCallable, Category = "Security")
	void EnableSecuritySystem();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Security")
	float AlertCooldown = 3.0f;

	// Default time (in seconds) the security system stays disabled
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Security", meta = (ClampMin = "0.1"))
	float DefaultDisableDuration = 15.0f;

private:
	void ReactivateSecuritySystem();
	FTimerHandle ReactivateTimerHandle;

	bool bIsSystemActive = true;
	float LastAlertTime = -100.0f;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
