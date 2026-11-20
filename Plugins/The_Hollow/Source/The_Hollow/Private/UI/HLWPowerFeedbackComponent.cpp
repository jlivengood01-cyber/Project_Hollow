#include "UI/HLWPowerFeedbackComponent.h"

#include "Blueprint/UserWidget.h"
#include "Electrical/HLWFacilityPowerSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "UI/HLWPowerNotificationWidget.h"
#include "UI/HLWPowerStatusWidget.h"

#define LOCTEXT_NAMESPACE "HLWPowerFeedback"

namespace
{
	// Area names listed before "+N more"
	constexpr int32 MaxNamedAreas = 2;
}

UHLWPowerFeedbackComponent::UHLWPowerFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	NotificationWidgetClass = UHLWPowerNotificationWidget::StaticClass();
	StatusWidgetClass = UHLWPowerStatusWidget::StaticClass();
}

void UHLWPowerFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>())
	{
		// The first resolve already happened, so seed from it
		for (const FName& ID : Power->GetCircuitIDs())
		{
			LastStates.Add(ID, Power->GetCircuitState(ID));
		}
		Power->OnCircuitStateChanged.AddDynamic(this, &ThisClass::HandleCircuitStateChanged);
	}
}

void UHLWPowerFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	if (UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>())
	{
		Power->OnCircuitStateChanged.RemoveDynamic(this, &ThisClass::HandleCircuitStateChanged);
	}
	for (UUserWidget* Widget : TArray<UUserWidget*>{ NotificationWidget.Get(), StatusWidget.Get() })
	{
		if (Widget)
		{
			Widget->RemoveFromParent();
		}
	}
	Super::EndPlay(EndPlayReason);
}

void UHLWPowerFeedbackComponent::HandleCircuitStateChanged(FName CircuitID, EHLWPowerState State, bool bInitial)
{
	const EHLWPowerState Previous = LastStates.Contains(CircuitID) ? LastStates[CircuitID] : EHLWPowerState::Unpowered;
	LastStates.Add(CircuitID, State);
	if (bInitial || Previous == State)
	{
		return;
	}

	TOptional<EHLWPowerNoticeType> Notice;
	if (State == EHLWPowerState::Faulted)
	{
		Notice = EHLWPowerNoticeType::Fault;
	}
	else if (State == EHLWPowerState::Powered)
	{
		Notice = EHLWPowerNoticeType::Restored;
	}
	else if (State == EHLWPowerState::Backup)
	{
		Notice = EHLWPowerNoticeType::Backup;
	}
	else if (HLWPower::IsLive(Previous) && (State == EHLWPowerState::Unpowered || bNoticeOnSwitchOff))
	{
		Notice = EHLWPowerNoticeType::Failure;
	}

	if (!Notice.IsSet())
	{
		return;
	}
	PendingNotices.FindOrAdd(Notice.GetValue()).AddUnique(CircuitID);
	if (!bFlushQueued)
	{
		bFlushQueued = true;
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::FlushNotices);
	}
}

void UHLWPowerFeedbackComponent::FlushNotices()
{
	bFlushQueued = false;
	if (PendingNotices.IsEmpty())
	{
		return;
	}

	// Worst news wins
	EHLWPowerNoticeType Type = EHLWPowerNoticeType::Restored;
	for (const TPair<EHLWPowerNoticeType, TArray<FName>>& Pair : PendingNotices)
	{
		Type = FMath::Max(Type, Pair.Key);
	}

	TArray<FString> Areas;
	for (const FName& CircuitID : PendingNotices[Type])
	{
		Areas.AddUnique(GetAreaName(CircuitID).ToString());
	}
	PendingNotices.Reset();

	FString AreaLine = FString::Join(TArrayView<const FString>(Areas.GetData(), FMath::Min(Areas.Num(), MaxNamedAreas)), TEXT(", "));
	if (Areas.Num() > MaxNamedAreas)
	{
		AreaLine += FString::Printf(TEXT(" +%d more"), Areas.Num() - MaxNamedAreas);
	}
	ShowNotice(Type, FText::FromString(AreaLine));
}

void UHLWPowerFeedbackComponent::ShowNotice(EHLWPowerNoticeType Type, const FText& AreaText)
{
	if (UHLWPowerNotificationWidget* Widget = GetOrCreateWidget(NotificationWidgetClass, NotificationWidget))
	{
		Widget->ShowPowerNotice(Type, AreaText, NoticeDuration);
	}
}

void UHLWPowerFeedbackComponent::ToggleStatusPanel()
{
	UHLWPowerStatusWidget* Widget = GetOrCreateWidget(StatusWidgetClass, StatusWidget);
	if (!Widget)
	{
		return;
	}
	if (Widget->IsVisible())
	{
		Widget->HideNow();
	}
	else
	{
		Widget->ShowFor(0.f);
		Widget->Refresh();
	}
}

FText UHLWPowerFeedbackComponent::GetAreaName(FName CircuitID) const
{
	const UHLWFacilityPowerSubsystem* Power = GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>();
	FHLWCircuitData Data;
	if (Power && Power->GetCircuitData(CircuitID, Data) && !Data.Area.IsNone())
	{
		return FText::FromName(Data.Area);
	}
	return Power ? Power->GetCircuitDisplayName(CircuitID) : FText::FromName(CircuitID);
}

APlayerController* UHLWPowerFeedbackComponent::GetLocalPlayerController() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	return PC && PC->IsLocalController() ? PC : nullptr;
}

template <typename WidgetT>
WidgetT* UHLWPowerFeedbackComponent::GetOrCreateWidget(TSubclassOf<WidgetT> WidgetClass, TObjectPtr<WidgetT>& Cache)
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
