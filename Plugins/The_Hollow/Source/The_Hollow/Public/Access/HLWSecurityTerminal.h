#pragma once

#include "CoreMinimal.h"
#include "Access/HLWAccessDeviceBase.h"
#include "Access/HLWAccessTypes.h"
#include "HLWInteractableInterface.h"
#include "HLWSecurityTerminal.generated.h"

USTRUCT(BlueprintType)
struct FHLWTerminalFunction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Terminal")
	FName FunctionID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Terminal")
	FText DisplayName;

	// Off means the login credential is enough
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Terminal")
	bool bRequiresPermission = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Terminal", meta = (EditCondition = "bRequiresPermission"))
	FHLWAccessRequirement Permission;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHLWOnTerminalLogin, AActor*, User);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHLWOnTerminalLogout);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHLWOnTerminalFunction, FName, FunctionID, AActor*, User);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHLWOnTerminalFunctionDenied, FName, FunctionID, EHLWAccessResult, Result);

// Terminal login and per-function permissions. The Security System performs the functions.
UCLASS()
class THE_HOLLOW_API AHLWSecurityTerminal : public AHLWAccessDeviceBase, public IHLWInteractableInterface
{
	GENERATED_BODY()

public:
	AHLWSecurityTerminal();

	UFUNCTION(BlueprintCallable, Category = "HLW|Terminal")
	EHLWAccessResult LogIn(AActor* User);

	UFUNCTION(BlueprintCallable, Category = "HLW|Terminal")
	void LogOut();

	// Logs in first if needed, then checks the function's own permission
	UFUNCTION(BlueprintCallable, Category = "HLW|Terminal")
	EHLWAccessResult UseFunction(FName FunctionID, AActor* User);

	UFUNCTION(BlueprintPure, Category = "HLW|Terminal")
	bool IsLoggedIn() const { return bLoggedIn; }

	UPROPERTY(BlueprintAssignable, Category = "HLW|Terminal")
	FHLWOnTerminalLogin OnLoggedIn;

	UPROPERTY(BlueprintAssignable, Category = "HLW|Terminal")
	FHLWOnTerminalLogout OnLoggedOut;

	// The Security System binds here to disable cameras, alarms and so on
	UPROPERTY(BlueprintAssignable, Category = "HLW|Terminal")
	FHLWOnTerminalFunction OnFunctionAuthorized;

	UPROPERTY(BlueprintAssignable, Category = "HLW|Terminal")
	FHLWOnTerminalFunctionDenied OnFunctionDenied;

	// Interact steps through log in, each function, log out; a test stand-in for the terminal UI
	virtual void Interact_Implementation(AActor* InteractingActor) override;
	virtual FText GetInteractionText_Implementation() override;
	virtual bool CanInteract_Implementation() override { return true; }

protected:
	virtual void BeginPlay() override;
	virtual void OnDeviceStateChanged(bool bPowerChanged) override;

	// Login credential
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Terminal")
	FHLWFacilityAccessData AccessData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Terminal")
	TArray<FHLWTerminalFunction> Functions;

	UPROPERTY(EditAnywhere, Category = "HLW|Terminal")
	bool bShowDebugResult = true;

private:
	bool IsAvailable() const;
	void ShowDebug(const FString& Label, EHLWAccessResult Result) const;

	bool bLoggedIn = false;
	int32 LastFunctionIndex = INDEX_NONE;
};
