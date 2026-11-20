#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HLWTestCharacter.generated.h"

class UCameraComponent;
class UHLWAccessFeedbackComponent;
class UHLWItemHolderComponent;
class UHLWKeycardInventoryComponent;
class UHLWPowerFeedbackComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

// First-person test pawn. Builds its own input, so it works without any setup.
// WASD to move, mouse to look, E to interact, F5 to save, F9 to load, P for the power status panel.
UCLASS()
class THE_HOLLOW_API AHLWTestCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AHLWTestCharacter();

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Test")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Test")
	TObjectPtr<UHLWKeycardInventoryComponent> KeycardInventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Test")
	TObjectPtr<UHLWAccessFeedbackComponent> AccessFeedback;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Test")
	TObjectPtr<UHLWItemHolderComponent> ItemHolder;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Test")
	TObjectPtr<UHLWPowerFeedbackComponent> PowerFeedback;

	UPROPERTY(EditAnywhere, Category = "HLW|Test", meta = (ClampMin = "0"))
	float WalkSpeed = 400.f;

	UPROPERTY(EditAnywhere, Category = "HLW|Test", meta = (ClampMin = "0.01"))
	float LookSensitivity = 1.f;

	UPROPERTY(EditAnywhere, Category = "HLW|Test", meta = (ClampMin = "0"))
	float InteractRange = 250.f;

	UPROPERTY(EditAnywhere, Category = "HLW|Test")
	bool bDrawInteractTrace = true;

	UPROPERTY(EditAnywhere, Category = "HLW|Test")
	FString TestSaveSlot = TEXT("HLW_AccessTest");

	UPROPERTY(EditAnywhere, Category = "HLW|Test")
	FString PowerSaveSlot = TEXT("HLW_PowerTest");

private:
	void BuildInput();
	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleInteract();
	void HandleSave();
	void HandleLoad();
	void HandleTogglePowerPanel();
	void Report(const FString& Message) const;

	// Created at runtime, no assets needed
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> InputContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SaveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LoadAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PowerPanelAction;
};
