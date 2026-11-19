// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HLW_ISecurityInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UHLW_ISecurityInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class THE_HOLLOW_API IHLW_ISecurityInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	// Called by the player (e.g., via terminal, hacking tool, or interaction trace)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Security")
	void DisableSecuritySystem(float OverrideDuration = -1.0f);
};
