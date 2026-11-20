// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HLWDebugLibrary.generated.h"

/**
 * Shared on-screen debug output
 * Off by default; enable in-game with console command: HLW.Debug 1
 * Compiled out of shipping builds.
 */
UCLASS()
class THE_HOLLOW_API UHLWDebugLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Print a debug message on screen and to the log.
	 * Key: messages with the same key replace each other (use for live status lines). None = new line each time.
	 */
	UFUNCTION(BlueprintCallable, Category = "HLW|Debug", meta = (DevelopmentOnly, AdvancedDisplay = "Duration,Key"))
	static void HLWDebugMessage(const FString& Message, FLinearColor Color = FLinearColor(1.f, 0.85f, 0.f),
		float Duration = 4.f, FName Key = NAME_None);

	UFUNCTION(BlueprintPure, Category = "HLW|Debug")
	static bool IsHLWDebugEnabled();
};
