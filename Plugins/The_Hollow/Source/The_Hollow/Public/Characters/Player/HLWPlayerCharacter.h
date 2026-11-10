
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HLWPlayerCharacter.generated.h"

class UCameraComponent;
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

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HLW|Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	// Input actions: assigned in BP_HLW_PlayerCharacter
	UPROPERTY(EditDefaultsOnly, Category = "HLW|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "HLW|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "HLW|Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, Category = "HLW|Input")
	TObjectPtr<UInputAction> CrouchAction;

	// Tuning: designers adjust these in the Blueprint
	UPROPERTY(EditDefaultsOnly, Category = "HLW|Movement", meta = (ClampMin = "0"))
	float WalkSpeed = 250.f;

	UPROPERTY(EditDefaultsOnly, Category = "HLW|Movement", meta = (ClampMin = "0"))
	float SprintSpeed = 450.f;

	UPROPERTY(EditDefaultsOnly, Category = "HLW|Movement", meta = (ClampMin = "0"))
	float CrouchSpeed = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "HLW|Camera", meta = (ClampMin = "0.01"))
	float LookSensitivity = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "HLW|Movement")
	bool bToggleCrouch = true;

private:
	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleSprintStarted();
	void HandleSprintCompleted();
	void HandleCrouchStarted();
	void HandleCrouchCompleted();

	bool bIsSprinting = false;
};