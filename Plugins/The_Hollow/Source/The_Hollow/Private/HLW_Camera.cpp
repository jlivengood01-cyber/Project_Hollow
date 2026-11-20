// Fill out your copyright notice in the Description page of Project Settings.


#include "HLW_Camera.h"

// Sets default values
AHLW_Camera::AHLW_Camera()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AHLW_Camera::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AHLW_Camera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

