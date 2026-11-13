// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HLWInteractableInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UHLWInteractableInterface : public UInterface
{
    GENERATED_BODY()
};

class THE_HOLLOW_API IHLWInteractableInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HLW|Interaction")
    void Interact(AActor* InteractingActor);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HLW|Interaction")
    FText GetInteractionText();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HLW|Interaction")
    bool CanInteract();
};
