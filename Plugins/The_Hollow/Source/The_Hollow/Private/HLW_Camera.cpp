// Fill out your copyright notice in the Description page of Project Settings.


#include "HLW_Camera.h"
#include "Components/StaticMeshComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

// Sets default values
AHLW_Camera::AHLW_Camera()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootComp);

	CameraMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CameraMesh"));
	CameraMesh->SetupAttachment(RootComp);

	// Set up AI Sight Perception
	AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComp"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	SightConfig->SightRadius = 1500.0f;
	SightConfig->LoseSightRadius = 1700.0f;
	SightConfig->PeripheralVisionAngleDegrees = 45.0f;
	SightConfig->SetMaxAge(0.5f);

	// Detect all affiliation types (needed when player doesn't have an IGenericTeamAgentInterface)
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	AIPerceptionComp->ConfigureSense(*SightConfig);
	AIPerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());

}

// Called when the game starts or when spawned
void AHLW_Camera::BeginPlay()
{
	Super::BeginPlay();

	//If camera has pivot point (can pan)m grab the initial rotation.
	if (CameraPivot)
	{
		InitialPivotRotation = CameraPivot->GetRelativeRotation();
	}

	// Bind perception callback
	if (AIPerceptionComp)
	{
		AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AHLW_Camera::HandleTargetPerceptionUpdated);
	}

	// Grab SecurityManager from level if not manually assigned in Details panel
	if (!SecurityManagerRef)
	{
		SecurityManagerRef = Cast<AHLW_Security_System_Manager>(
			UGameplayStatics::GetActorOfClass(GetWorld(), AHLW_Security_System_Manager::StaticClass())
		);
	}

	if (SecurityManagerRef)
	{
		SecurityManagerRef->OnSecuritySystemStateChanged.AddDynamic(
			this, &AHLW_Camera::HandleSecuritySystemStateChanged
		);

		// Sync initial state in case the system starts disabled
		HandleSecuritySystemStateChanged(SecurityManagerRef->IsSecuritySystemActive());
	}
}

void AHLW_Camera::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// Filter specifically for the player pawn
	//TODO: refactor this to specifically choose the Hollow Player Character once class is implemented
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Actor || Actor != PlayerPawn)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		TrackedPlayer = Actor;
		OnDetectionStateChanged(true);

		if (DetectionDelay <= 0.0f)
		{
			ConfirmAndReportPlayer();
		}
		else if (!GetWorldTimerManager().IsTimerActive(DetectionTimerHandle))
		{
			GetWorldTimerManager().SetTimer(
				DetectionTimerHandle,
				this,
				&AHLW_Camera::ConfirmAndReportPlayer,
				DetectionDelay,
				false
			);
		}
	}
	else
	{
		// Player broke line-of-sight or exited the cone
		TrackedPlayer = nullptr;
		GetWorldTimerManager().ClearTimer(DetectionTimerHandle);
		OnDetectionStateChanged(false);
	}
}

void AHLW_Camera::ConfirmAndReportPlayer()
{
	if (!SecurityManagerRef || !TrackedPlayer || !AIPerceptionComp)
	{
		return;
	}

	// Double-check that the player is still currently perceived in the sight cone
	TArray<AActor*> CurrentlyPerceivedActors;
	AIPerceptionComp->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), CurrentlyPerceivedActors);

	if (CurrentlyPerceivedActors.Contains(TrackedPlayer))
	{
		SecurityManagerRef->ReportPlayerSpotted(
			TrackedPlayer->GetActorLocation(),
			AssignedZone,
			this
		);
	}
}

// Called every frame
void AHLW_Camera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bEnablePanning || !CameraPivot || MaxPanAngle <= 0.0f) return;

	if (bPausedAtEdge)
	{
		CurrentPauseTimer -= DeltaTime;
		if (CurrentPauseTimer <= 0.0f)
		{
			bPausedAtEdge = false;
		}
		return;
	}

	//Calculate pan pathing/distance along path
	//By setting the initial rotation in the level, the camera will move x degrees one way, 
	//and then x * 2 degrees the other way, and repeat. x = MaxPanAngle
	const float PrevSin = FMath::Sin(PanPhase);
	PanPhase += DeltaTime * PanSpeed;
	const float NewSin = FMath::Sin(PanPhase);

	// Detect peak (+1 or -1) to trigger optional edge pause
	if (EdgePauseDuration > 0.0f && ((PrevSin < 0.999f && NewSin >= 0.999f) || (PrevSin > -0.999f && NewSin <= -0.999f)))
	{
		bPausedAtEdge = true;
		CurrentPauseTimer = EdgePauseDuration;
	}

	FRotator NewRot = InitialPivotRotation;
	NewRot.Yaw += NewSin * MaxPanAngle;
	CameraPivot->SetRelativeRotation(NewRot);
}

void AHLW_Camera::GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	if (CameraPivot)
	{
		OutLocation = CameraPivot->GetComponentLocation();
		OutRotation = CameraPivot->GetComponentRotation();
		return;
	}
	Super::GetActorEyesViewPoint(OutLocation, OutRotation);
}

void AHLW_Camera::HandleSecuritySystemStateChanged(bool bIsActive)
{
	if (AIPerceptionComp)
	{
		AIPerceptionComp->SetActive(bIsActive);
	}

	if (!bIsActive)
	{
		// Cancel any active player-detection confirmation timer
		GetWorldTimerManager().ClearTimer(DetectionTimerHandle);
	}

	// Forward to Blueprint for lights/lens materials
	OnCameraPowerStateChanged(bIsActive);
}
