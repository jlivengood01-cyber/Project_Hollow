// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataAsset.h"
#include "HLW_Security_System_Manager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnSecurityAlertSignature,
	FVector, AlertLocation,
	UPrimaryDataAsset*, AlertZone
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

	// Function called by Cameras / Watcher AI
	UFUNCTION(BlueprintCallable, Category = "Security")
	void ReportPlayerSpotted(FVector PlayerLocation, UPrimaryDataAsset* Zone, AActor* SourceSensor = nullptr);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Security")
	float AlertCooldown = 3.0f;

private:
	float LastAlertTime = -100.0f;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
