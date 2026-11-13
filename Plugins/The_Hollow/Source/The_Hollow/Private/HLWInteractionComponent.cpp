// Fill out your copyright notice in the Description page of Project Settings.


#include "HLWInteractionComponent.h"

// Sets default values
AHLWInteractionComponent::AHLWInteractionComponent()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AHLWInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AHLWInteractionComponent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

