#include "UI/HLWAccessFeedbackComponent.h"

#include "Access/HLWAccessMessageSet.h"
#include "Access/HLWFacilityAccessSubsystem.h"
#include "Access/HLWKeycardInventoryComponent.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HLWInteractableInterface.h"
#include "TimerManager.h"
#include "UI/HLWAccessMessageWidget.h"
#include "UI/HLWInteractionPromptWidget.h"
#include "UI/HLWKeycardNotificationWidget.h"

#define LOCTEXT_NAMESPACE "HLWAccessFeedback"

UHLWAccessFeedbackComponent::UHLWAccessFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	GrantedWidgetClass = UHLWAccessMessageWidget::StaticClass();
	DeniedWidgetClass = UHLWAccessMessageWidget::StaticClass();
	OfflineWidgetClass = UHLWAccessMessageWidget::StaticClass();
	KeycardNotificationClass = UHLWKeycardNotificationWidget::StaticClass();
	PromptWidgetClass = UHLWInteractionPromptWidget::StaticClass();
	PromptKeyLabel = LOCTEXT("PromptKey", "E");
}

void UHLWAccessFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UHLWKeycardInventoryComponent* Inventory = GetOwner()->FindComponentByClass<UHLWKeycardInventoryComponent>())
	{
		Inventory->OnKeycardAdded.AddDynamic(this, &ThisClass::HandleKeycardAdded);
	}
	if (UHLWFacilityAccessSubsystem* Access = GetWorld()->GetSubsystem<UHLWFacilityAccessSubsystem>())
	{
		Access->OnAccessResolved.AddDynamic(this, &ThisClass::HandleAccessResolved);
	}
	if (bShowInteractionPrompt)
	{
		GetWorld()->GetTimerManager().SetTimer(PromptTimer, this, &ThisClass::UpdatePrompt, PromptInterval, true);
	}
}

void UHLWAccessFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(PromptTimer);

	if (UHLWKeycardInventoryComponent* Inventory = GetOwner()->FindComponentByClass<UHLWKeycardInventoryComponent>())
	{
		Inventory->OnKeycardAdded.RemoveDynamic(this, &ThisClass::HandleKeycardAdded);
	}
	if (UHLWFacilityAccessSubsystem* Access = GetWorld()->GetSubsystem<UHLWFacilityAccessSubsystem>())
	{
		Access->OnAccessResolved.RemoveDynamic(this, &ThisClass::HandleAccessResolved);
	}

	for (UUserWidget* Widget : TArray<UUserWidget*>{ GrantedWidget.Get(), DeniedWidget.Get(), OfflineWidget.Get(), NotificationWidget.Get(), PromptWidget.Get() })
	{
		if (Widget)
		{
			Widget->RemoveFromParent();
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UHLWAccessFeedbackComponent::HandleKeycardAdded(UHLWKeycardDefinition* Keycard)
{
	if (UHLWKeycardNotificationWidget* Widget = GetOrCreateWidget(KeycardNotificationClass, NotificationWidget))
	{
		Widget->ShowKeycard(Keycard, KeycardNotificationDuration);
	}
}

void UHLWAccessFeedbackComponent::HandleAccessResolved(FName DeviceID, EHLWAccessResult Result)
{
	if (!GetLocalPlayerController())
	{
		return;
	}

	UHLWAccessMessageWidget* Widget = nullptr;
	FText Title;
	if (Result == EHLWAccessResult::Granted)
	{
		Widget = GetOrCreateWidget(GrantedWidgetClass, GrantedWidget);
		Title = LOCTEXT("GrantedTitle", "Access Granted");
	}
	else if (HLWAccess::IsOfflineResult(Result))
	{
		Widget = GetOrCreateWidget(OfflineWidgetClass, OfflineWidget);
		Title = LOCTEXT("OfflineTitle", "Reader Offline");
	}
	else
	{
		Widget = GetOrCreateWidget(DeniedWidgetClass, DeniedWidget);
		Title = HLWAccess::IsCredentialResult(Result)
			? LOCTEXT("DeniedTitle", "Access Denied")
			: LOCTEXT("LockedTitle", "Locked");
	}

	// One access message on screen at a time
	for (UHLWAccessMessageWidget* Other : { GrantedWidget.Get(), DeniedWidget.Get(), OfflineWidget.Get() })
	{
		if (Other && Other != Widget)
		{
			Other->HideNow();
		}
	}

	if (Widget)
	{
		const FText Message = MessageSet ? MessageSet->GetMessage(Result) : UHLWAccessMessageSet::GetDefaultMessage(Result);
		Widget->ShowAccessMessage(Result, Title, Message, AccessMessageDuration);
	}
}

void UHLWAccessFeedbackComponent::UpdatePrompt()
{
	FText Prompt;

	if (APlayerController* PC = GetLocalPlayerController())
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

		FHitResult Hit;
		const FCollisionQueryParams Params(SCENE_QUERY_STAT(HLWPromptTrace), false, GetOwner());
		const FVector End = ViewLocation + ViewRotation.Vector() * PromptRange;

		AActor* Target = GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, End, ECC_Visibility, Params) ? Hit.GetActor() : nullptr;
		if (Target && Target->Implements<UHLWInteractableInterface>())
		{
			const FText Text = IHLWInteractableInterface::Execute_GetInteractionText(Target);
			const bool bShowKey = !PromptKeyLabel.IsEmpty() && IHLWInteractableInterface::Execute_CanInteract(Target);

			// Key only shows when the action is available, so "Locked" reads on its own
			Prompt = bShowKey && !Text.IsEmpty()
				? FText::Format(LOCTEXT("PromptFormat", "[{0}] {1}"), PromptKeyLabel, Text)
				: Text;
		}
	}

	if (Prompt.EqualTo(CurrentPrompt))
	{
		return;
	}
	CurrentPrompt = Prompt;

	if (Prompt.IsEmpty())
	{
		if (PromptWidget)
		{
			PromptWidget->ClearPrompt();
		}
	}
	else if (UHLWInteractionPromptWidget* Widget = GetOrCreateWidget(PromptWidgetClass, PromptWidget))
	{
		Widget->SetPrompt(Prompt);
	}
}

APlayerController* UHLWAccessFeedbackComponent::GetLocalPlayerController() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	return PC && PC->IsLocalController() ? PC : nullptr;
}

template <typename WidgetT>
WidgetT* UHLWAccessFeedbackComponent::GetOrCreateWidget(TSubclassOf<WidgetT> WidgetClass, TObjectPtr<WidgetT>& Cache)
{
	if (!Cache && WidgetClass)
	{
		if (APlayerController* PC = GetLocalPlayerController())
		{
			Cache = CreateWidget<WidgetT>(PC, WidgetClass);
			if (Cache)
			{
				Cache->AddToViewport(WidgetZOrder);
			}
		}
	}
	return Cache;
}

#undef LOCTEXT_NAMESPACE
