// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Perception/AIPerceptionTypes.h"
#include "HLW_Security_System_Manager.h"
#include "HLW_Camera.generated.h"

class UStaticMeshComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UPrimaryDataAsset;
class ASecurityManager;

UCLASS()
class THE_HOLLOW_API AHLW_Camera : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHLW_Camera();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CameraMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	// Designer Settings
	//Each zone is a separate PrimaryDataAsset, 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Security")
	TObjectPtr<UPrimaryDataAsset> AssignedZone;

	// Optional manual reference; if left null, BeginPlay finds ASecurityManager automatically
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Security")
	TObjectPtr<AHLW_Security_System_Manager> SecurityManagerRef;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> CameraPivot;

	// Total angle in degrees from center (e.g., 45 means -45 to +45)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Panning", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float MaxPanAngle = 45.0f;

	// Speed of the sweep
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Panning", meta = (ClampMin = "0.0"))
	float PanSpeed = 1.0f;

	// Pause duration at each end of the sweep (in seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Panning", meta = (ClampMin = "0.0"))
	float EdgePauseDuration = 1.0f;

	//Enable the designer to turn panning on/off
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Panning")
	bool bEnablePanning = true;

	// Must be marked UFUNCTION() to bind to a dynamic multicast delegate
	UFUNCTION()
	void HandleSecuritySystemStateChanged(bool bIsActive);

	// Lets the Blueprint camera turn off spotlights/materials easily
	UFUNCTION(BlueprintImplementableEvent, Category = "Security")
	void OnCameraPowerStateChanged(bool bIsActive);

	// Set to 0.0f for instant alert, or > 0.0f for a brief grace period before reporting
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Security", meta = (ClampMin = "0.0"))
	float DetectionDelay = 1.5f;

	// Optional Blueprint hooks for changing light colors, playing beeps, etc.
	UFUNCTION(BlueprintImplementableEvent, Category = "Security")
	void OnDetectionStateChanged(bool bIsSpottingPlayer);

private:
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void ConfirmAndReportPlayer();

	UPROPERTY()
	TObjectPtr<AActor> TrackedPlayer;

	FTimerHandle DetectionTimerHandle;

	FRotator InitialPivotRotation;
	float PanPhase = 0.0f;
	float CurrentPauseTimer = 0.0f;
	bool bPausedAtEdge = false;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//Alters the sight cone to align with camera pivot point
	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;

};
