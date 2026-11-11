#include "Characters/Player/HLWPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputActionValue.h"

AHLWPlayerCharacter::AHLWPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false; // movement component ticks itself

	bUseControllerRotationYaw = true; // First-person

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f)); // Eye height
	FirstPersonCamera->bUsePawnControlRotation = true;

	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
}

void AHLWPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = WalkSpeed;
	Movement->MaxWalkSpeedCrouched = CrouchSpeed;
}

void AHLWPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	if (MoveAction)
	{
		Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::HandleMove);
	}
	if (LookAction)
	{
		Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::HandleLook);
	}
	if (SprintAction)
	{
		Input->BindAction(SprintAction, ETriggerEvent::Started, this, &ThisClass::HandleSprintStarted);
		Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &ThisClass::HandleSprintCompleted);
	}
	if (CrouchAction)
	{
		Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &ThisClass::HandleCrouchStarted);
		Input->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ThisClass::HandleCrouchCompleted);
	}
}

void AHLWPlayerCharacter::HandleMove(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), Axis.Y);
	AddMovementInput(GetActorRightVector(), Axis.X);
}

void AHLWPlayerCharacter::HandleLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>() * LookSensitivity;
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void AHLWPlayerCharacter::HandleSprintStarted()
{
	bIsSprinting = true;
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AHLWPlayerCharacter::HandleSprintCompleted()
{
	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AHLWPlayerCharacter::HandleCrouchStarted()
{
	if (bToggleCrouch && bIsCrouched)
	{
		UnCrouch();
		return;
	}
	Crouch();
}

void AHLWPlayerCharacter::HandleCrouchCompleted()
{
	if (!bToggleCrouch)
	{
		UnCrouch();
	}
}