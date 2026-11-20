#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Access/HLWAccessTypes.h"
#include "HLWAccessControllable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UHLWAccessControllable : public UInterface
{
	GENERATED_BODY()
};

// Implemented by doors, elevators and terminals that a reader controls
class THE_HOLLOW_API IHLWAccessControllable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HLW|Access")
	void OnAccessGranted(AActor* Reader);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HLW|Access")
	void OnAccessDenied(AActor* Reader, EHLWAccessResult Result);

	// Device shares the reader's power circuit
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HLW|Access")
	void OnReaderPowerChanged(AActor* Reader, bool bPowered);

	// Lockdown or disabled access on the reader
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HLW|Access")
	void OnSecurityLockChanged(AActor* Reader, bool bLocked);

	// Saved unlock state applied on level start or load, without sounds or opening
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HLW|Access")
	void OnAccessStateRestored(AActor* Reader, bool bUnlocked);
};
