#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HLWAccessDeviceState.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UHLWAccessDeviceState : public UInterface
{
	GENERATED_BODY()
};

// Lets the access subsystem read power, network and fault state from a device
class THE_HOLLOW_API IHLWAccessDeviceState
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HLW|Access")
	bool IsPowered();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HLW|Access")
	bool IsNetworked();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HLW|Access")
	bool IsOperational();
};
