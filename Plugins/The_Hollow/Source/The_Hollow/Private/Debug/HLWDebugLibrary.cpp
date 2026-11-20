// Fill out your copyright notice in the Description page of Project Settings.


#include "Debug/HLWDebugLibrary.h"

#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogHLWDebug, Log, All);

#if !UE_BUILD_SHIPPING
namespace
{
	TAutoConsoleVariable<bool> CVarHLWDebug(
		TEXT("HLW.Debug"),
		false,
		TEXT("Show The Hollow on-screen debug messages. 0 = off, 1 = on."));
}
#endif

void UHLWDebugLibrary::HLWDebugMessage(const FString& Message, FLinearColor Color, float Duration, FName Key)
{
#if !UE_BUILD_SHIPPING
	if (!IsHLWDebugEnabled())
	{
		return;
	}

	UE_LOG(LogHLWDebug, Log, TEXT("%s"), *Message);

	if (GEngine)
	{
		const uint64 MessageKey = Key.IsNone() ? static_cast<uint64>(-1) : GetTypeHash(Key);
		GEngine->AddOnScreenDebugMessage(MessageKey, Duration, Color.ToFColor(true), Message);
	}
#endif
}

bool UHLWDebugLibrary::IsHLWDebugEnabled()
{
#if !UE_BUILD_SHIPPING
	return CVarHLWDebug.GetValueOnGameThread();
#else
	return false;
#endif
}

