#include "Characters/Test/HLWTestCharacter.h"

#include "Access/HLWAccessStateSubsystem.h"
#include "Access/HLWKeycardInventoryComponent.h"
#include "UI/HLWAccessFeedbackComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HLWInteractableInterface.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

namespace
{
	// Above the Arcade hub and game mappings so test keys always win
	constexpr int32 TestInputPriority = 100;

	UEnhancedInputLocalPlayerSubsystem* GetInputSubsystem(AController* InController)
	{
		const APlayerController* PC = Cast<APlayerController>(InController);
		return PC ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()) : nullptr;
	}
}

AHLWTestCharacter::AHLWTestCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	bUseControllerRotationYaw = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 64.f)); // Eye height
	Camera->bUsePawnControlRotation = true;

	KeycardInventory = CreateDefaultSubobject<UHLWKeycardInventoryComponent>(TEXT("KeycardInventory"));
	AccessFeedback = CreateDefaultSubobject<UHLWAccessFeedbackComponent>(TEXT("AccessFeedback"));
}

void AHLWTestCharacter::PostInitializeComponents()
{
	// Build before Super so input exists if the pawn is possessed during spawn
	BuildInput();
	Super::PostInitializeComponents();
}

void AHLWTestCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AHLWTestCharacter::BuildInput()
{
	MoveAction = NewObject<UInputAction>(this, TEXT("IA_TestMove"));
	MoveAction->ValueType = EInputActionValueType::Axis2D;

	LookAction = NewObject<UInputAction>(this, TEXT("IA_TestLook"));
	LookAction->ValueType = EInputActionValueType::Axis2D;

	InteractAction = NewObject<UInputAction>(this, TEXT("IA_TestInteract"));
	SaveAction = NewObject<UInputAction>(this, TEXT("IA_TestSave"));
	LoadAction = NewObject<UInputAction>(this, TEXT("IA_TestLoad"));

	InputContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Test"));

	// Swizzle puts W/S on the Y axis, Negate flips S/A
	auto MapMoveKey = [this](const FKey& Key, bool bSwizzle, bool bNegate)
	{
		FEnhancedActionKeyMapping& Mapping = InputContext->MapKey(MoveAction, Key);
		if (bSwizzle)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(InputContext));
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(InputContext));
		}
	};

	MapMoveKey(EKeys::W, true, false);
	MapMoveKey(EKeys::S, true, true);
	MapMoveKey(EKeys::D, false, false);
	MapMoveKey(EKeys::A, false, true);

	InputContext->MapKey(LookAction, EKeys::Mouse2D);
	InputContext->MapKey(InteractAction, EKeys::E);
	InputContext->MapKey(SaveAction, EKeys::F5);
	InputContext->MapKey(LoadAction, EKeys::F9);
}

void AHLWTestCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	if (!InputContext)
	{
		return;
	}
	if (UEnhancedInputLocalPlayerSubsystem* OldSubsystem = GetInputSubsystem(PreviousController))
	{
		OldSubsystem->RemoveMappingContext(InputContext);
	}
	if (UEnhancedInputLocalPlayerSubsystem* NewSubsystem = GetInputSubsystem(Controller))
	{
		NewSubsystem->AddMappingContext(InputContext, TestInputPriority);
	}
}

void AHLWTestCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::HandleMove);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::HandleLook);
	Input->BindAction(InteractAction, ETriggerEvent::Started, this, &ThisClass::HandleInteract);
	Input->BindAction(SaveAction, ETriggerEvent::Started, this, &ThisClass::HandleSave);
	Input->BindAction(LoadAction, ETriggerEvent::Started, this, &ThisClass::HandleLoad);
}

void AHLWTestCharacter::HandleSave()
{
	UHLWAccessStateSubsystem* State = UHLWAccessStateSubsystem::Get(this);
	Report(State && State->SaveToSlot(TestSaveSlot) ? TEXT("Access state saved") : TEXT("Access save failed"));
}

void AHLWTestCharacter::HandleLoad()
{
	UHLWAccessStateSubsystem* State = UHLWAccessStateSubsystem::Get(this);
	Report(State && State->LoadFromSlot(TestSaveSlot) ? TEXT("Access state loaded") : TEXT("No access save found"));
}

void AHLWTestCharacter::HandleMove(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), Axis.Y);
	AddMovementInput(GetActorRightVector(), Axis.X);
}

void AHLWTestCharacter::HandleLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>() * LookSensitivity;
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(-Axis.Y); // Mouse up looks up
}

void AHLWTestCharacter::HandleInteract()
{
	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Camera->GetForwardVector() * InteractRange;

	FHitResult Hit;
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(HLWTestInteract), false, this);
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

	if (bDrawInteractTrace)
	{
		DrawDebugLine(GetWorld(), Start, bHit ? Hit.ImpactPoint : End, bHit ? FColor::Green : FColor::Red, false, 1.f);
	}

	AActor* Target = Hit.GetActor();
	if (!Target)
	{
		Report(TEXT("Interact: nothing in range"));
		return;
	}
	if (!Target->Implements<UHLWInteractableInterface>())
	{
		Report(FString::Printf(TEXT("Interact: %s is not interactable"), *Target->GetName()));
		return;
	}
	if (!IHLWInteractableInterface::Execute_CanInteract(Target))
	{
		Report(FString::Printf(TEXT("Interact: %s returned CanInteract false"), *Target->GetName()));
		return;
	}

	IHLWInteractableInterface::Execute_Interact(Target, this);
	Report(FString::Printf(TEXT("Interact: used %s"), *Target->GetName()));
}

void AHLWTestCharacter::Report(const FString& Message) const
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, Message);
	}
}
