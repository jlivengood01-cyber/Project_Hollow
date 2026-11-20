
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/HLWHealthComponent.h"
#include "HLWPlayerCharacter.generated.h"

class UCameraComponent;
class UHLWAccessFeedbackComponent;
class UHLWKeycardInventoryComponent;
class UInputAction;
struct FInputActionValue;

UCLASS()
class THE_HOLLOW_API AHLWPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AHLWPlayerCharacter();

	UFUNCTION(BlueprintPure, Category = "HLW|Movement")
	bool IsSprinting() const { return bIsSprinting; }

	UFUNCTION(BlueprintPure, Category = "HLW|Health")
	UHLWHealthComponent* GetHealthComponent() const { return HealthComponent; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Health")
	TObjectPtr<UHLWHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Keycard")
	TObjectPtr<UHLWKeycardInventoryComponent> KeycardInventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Keycard")
	TObjectPtr<UHLWAccessFeedbackComponent> AccessFeedback;


	// Input actions: assigned in BP_HLW_PlayerCharacter
	UPROPERTY(EditDefaultsOnly, Category = "HLW|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "HLW|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "HLW|Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, Category = "HLW|Input")
	TObjectPtr<UInputAction> CrouchAction;

	// Adjust in the Blueprint
	UPROPERTY(EditDefaultsOnly, Category = "HLW|Movement", meta = (ClampMin = "0"))
	float WalkSpeed = 250.f;

	UPROPERTY(EditDefaultsOnly, Category = "HLW|Movement", meta = (ClampMin = "0"))
	float SprintSpeed = 450.f;

	UPROPERTY(EditDefaultsOnly, Category = "HLW|Movement", meta = (ClampMin = "0"))
	float CrouchSpeed = 125.f;

	UPROPERTY(EditDefaultsOnly, Category = "HLW|Camera", meta = (ClampMin = "0.01"))
	float LookSensitivity = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "HLW|Movement")
	bool bToggleCrouch = true;

	/** Speed multiplier while Injured */ 
	UPROPERTY(EditDefaultsOnly, Category = "HLW|Movement", meta = (ClampMin = "0", ClampMax = "1"))
	float InjuredSpeedMultiplier = 0.6f;

private:
	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleSprintStarted();
	void HandleSprintCompleted();
	void HandleCrouchStarted();
	void HandleCrouchCompleted();

	bool bIsSprinting = false;

	UFUNCTION()                                                              
	void HandleHealthStateChanged(EHLWHealthState NewState, EHLWHealthState OldState);

	void ApplyMovementSpeeds();

	float SpeedMultiplier = 1.f;
};