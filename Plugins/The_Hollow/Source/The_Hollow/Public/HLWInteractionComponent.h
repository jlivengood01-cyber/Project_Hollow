// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HLWInteractionComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class THE_HOLLOW_API UHLWInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
    UHLWInteractionComponent();

    UFUNCTION(BlueprintCallable, Category = "HLW|Interaction")
    void CheckForInteractable();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Interaction")
    float InteractionDistance = 250.0f;
};
