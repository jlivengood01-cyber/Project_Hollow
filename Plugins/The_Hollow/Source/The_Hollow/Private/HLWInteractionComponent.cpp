// Fill out your copyright notice in the Description page of Project Settings.


#include "HLWInteractionComponent.h"
#include "HLWInteractableInterface.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"

UHLWInteractionComponent::UHLWInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UHLWInteractionComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UHLWInteractionComponent::CheckForInteractable()
{
    // Get the player who owns this interaction component
    APawn* OwnerPawn = Cast<APawn>(GetOwner());

    if (!OwnerPawn)
    {
        return;
    }

    // Find the player's first-person camera
    UCameraComponent* Camera = OwnerPawn->FindComponentByClass<UCameraComponent>();

    if (!Camera)
    {
        return;
    }

    // Calculate how far the player can interact
    FVector Start = Camera->GetComponentLocation();
    FVector End = Start + Camera->GetForwardVector() * InteractionDistance;

    FHitResult Hit;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerPawn);

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        Hit,
        Start,
        End,
        ECC_Visibility,
        Params
    );

    if (bHit && Hit.GetActor() &&
        Hit.GetActor()->GetClass()->ImplementsInterface(UHLWInteractableInterface::StaticClass()))
    {
        AActor* InteractableActor = Hit.GetActor();

        if (IHLWInteractableInterface::Execute_CanInteract(InteractableActor))
        {
            UE_LOG(LogTemp, Warning, TEXT("Interactable detected: %s"), *InteractableActor->GetName());
        }
    }
}

