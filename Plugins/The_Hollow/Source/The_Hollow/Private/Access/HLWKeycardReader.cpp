#include "Access/HLWKeycardReader.h"

#include "Access/HLWAccessControllable.h"
#include "Access/HLWAccessStateSubsystem.h"
#include "Access/HLWFacilityAccessSubsystem.h"
#include "Access/HLWKeycardInventoryComponent.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "HLWKeycardReader"

AHLWKeycardReader::AHLWKeycardReader()
{
	Mesh->SetRelativeScale3D(FVector(0.05f, 0.15f, 0.25f));

	StatusLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StatusLight"));
	StatusLight->SetupAttachment(GetRootComponent());
	StatusLight->SetRelativeLocation(FVector(8.f, 0.f, 8.f));
	StatusLight->SetAttenuationRadius(150.f);
	StatusLight->SetCastShadows(false);

	Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	Audio->SetupAttachment(GetRootComponent());
	Audio->bAutoActivate = false;
}

void AHLWKeycardReader::BeginPlay()
{
	Super::BeginPlay();

	if (AccessData.DeviceID.IsNone())
	{
		AccessData.DeviceID = HLWAccess::MakeStableID(this);
	}
	if (ControlledActor && !ControlledActor->Implements<UHLWAccessControllable>())
	{
		UE_LOG(LogHLWAccess, Warning, TEXT("%s: %s does not implement HLWAccessControllable"), *GetName(), *ControlledActor->GetName());
	}
	if (UHLWFacilityAccessSubsystem* Access = GetWorld()->GetSubsystem<UHLWFacilityAccessSubsystem>())
	{
		Access->OnLockdownChanged.AddDynamic(this, &ThisClass::HandleLockdownChanged);
	}

	// Sync the controlled device with the starting state
	NotifyPower();
	NotifySecurityLock();

	if (UHLWAccessStateSubsystem* SaveState = UHLWAccessStateSubsystem::Get(this))
	{
		SaveState->OnStateRestored.AddDynamic(this, &ThisClass::HandleStateRestored);
		HandleStateRestored();
	}
	else
	{
		SetState(GetRestingState());
	}
}

void AHLWKeycardReader::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	if (UHLWFacilityAccessSubsystem* Access = GetWorld()->GetSubsystem<UHLWFacilityAccessSubsystem>())
	{
		Access->OnLockdownChanged.RemoveDynamic(this, &ThisClass::HandleLockdownChanged);
	}
	if (UHLWAccessStateSubsystem* SaveState = UHLWAccessStateSubsystem::Get(this))
	{
		SaveState->OnStateRestored.RemoveDynamic(this, &ThisClass::HandleStateRestored);
	}
	Super::EndPlay(EndPlayReason);
}

void AHLWKeycardReader::HandleStateRestored()
{
	const UHLWAccessStateSubsystem* SaveState = UHLWAccessStateSubsystem::Get(this);
	bUnlocked = SaveState && SaveState->IsDeviceUnlocked(AccessData.DeviceID);

	if (HasControllable())
	{
		IHLWAccessControllable::Execute_OnAccessStateRestored(ControlledActor, this, bUnlocked);
	}
	RefreshRestingState();
}

void AHLWKeycardReader::Interact_Implementation(AActor* InteractingActor)
{
	if (State == EHLWReaderState::Scanning)
	{
		return;
	}

	PendingUser = InteractingActor;
	GetWorldTimerManager().ClearTimer(ResultTimer);

	// An unpowered reader can't scan, report straight away
	if (!IsAvailable() || ScanDuration <= 0.f)
	{
		ResolveAccess();
		return;
	}

	SetState(EHLWReaderState::Scanning);
	PlaySound(ScanSound);
	GetWorldTimerManager().SetTimer(ScanTimer, this, &ThisClass::ResolveAccess, ScanDuration, false);
}

FText AHLWKeycardReader::GetInteractionText_Implementation()
{
	return LOCTEXT("UseReader", "Use card reader");
}

bool AHLWKeycardReader::CanInteract_Implementation()
{
	return State != EHLWReaderState::Scanning;
}

void AHLWKeycardReader::ResolveAccess()
{
	AActor* User = PendingUser.Get();
	PendingUser.Reset();

	UHLWFacilityAccessSubsystem* Access = GetWorld()->GetSubsystem<UHLWFacilityAccessSubsystem>();
	if (!Access)
	{
		ReturnToIdle();
		return;
	}

	const UHLWKeycardInventoryComponent* Inventory = User ? User->FindComponentByClass<UHLWKeycardInventoryComponent>() : nullptr;
	const EHLWAccessResult Result = Access->RequestAccess(Inventory, AccessData, this);
	const bool bControllable = HasControllable();

	if (Result == EHLWAccessResult::Granted)
	{
		bUnlocked = true;
		if (UHLWAccessStateSubsystem* SaveState = UHLWAccessStateSubsystem::Get(this))
		{
			SaveState->SetDeviceUnlocked(AccessData.DeviceID, true);
		}

		SetState(EHLWReaderState::Granted);
		PlaySound(GrantedSound);
		if (bControllable)
		{
			IHLWAccessControllable::Execute_OnAccessGranted(ControlledActor, this);
		}
	}
	else
	{
		const bool bOffline = HLWAccess::IsOfflineResult(Result);
		const EHLWReaderState FailState = bOffline ? EHLWReaderState::Offline
			: HLWAccess::IsCredentialResult(Result) ? EHLWReaderState::Denied
			: EHLWReaderState::Locked;

		SetState(FailState);
		PlaySound(bOffline ? OfflineSound : DeniedSound);
		if (bControllable)
		{
			IHLWAccessControllable::Execute_OnAccessDenied(ControlledActor, this, Result);
		}
	}

	OnAccessResult.Broadcast(Result);

	if (bShowDebugResult && GEngine)
	{
		const FColor Color = Result == EHLWAccessResult::Granted ? FColor::Green : FColor::Red;
		GEngine->AddOnScreenDebugMessage(-1, 3.f, Color, FString::Printf(TEXT("%s: %s"), *GetName(), *UEnum::GetDisplayValueAsText(Result).ToString()));
	}

	GetWorldTimerManager().SetTimer(ResultTimer, this, &ThisClass::ReturnToIdle, ResultDisplayTime, false);
}

void AHLWKeycardReader::ReturnToIdle()
{
	SetState(GetRestingState());
}

void AHLWKeycardReader::RefreshRestingState()
{
	if (State != EHLWReaderState::Scanning)
	{
		GetWorldTimerManager().ClearTimer(ResultTimer);
		ReturnToIdle();
	}
}

EHLWReaderState AHLWKeycardReader::GetRestingState() const
{
	if (!IsAvailable())
	{
		return EHLWReaderState::Offline;
	}
	if (IsLockedBySystem())
	{
		return EHLWReaderState::Locked;
	}
	return bUnlocked ? EHLWReaderState::Granted : EHLWReaderState::Idle;
}

bool AHLWKeycardReader::IsLockedBySystem() const
{
	if (AccessData.bAccessDisabled)
	{
		return true;
	}
	const UHLWFacilityAccessSubsystem* Access = GetWorld() ? GetWorld()->GetSubsystem<UHLWFacilityAccessSubsystem>() : nullptr;
	return Access && Access->IsZoneLockedDown(AccessData.LockdownZone);
}

void AHLWKeycardReader::HandleLockdownChanged(FName Zone, bool bActive)
{
	if (Zone == AccessData.LockdownZone)
	{
		RefreshRestingState();
		NotifySecurityLock();
	}
}

void AHLWKeycardReader::OnDeviceStateChanged(bool bPowerChanged)
{
	RefreshRestingState();
	if (bPowerChanged)
	{
		NotifyPower();
	}
}

void AHLWKeycardReader::SetAccessDisabled(bool bDisabled)
{
	AccessData.bAccessDisabled = bDisabled;
	RefreshRestingState();
	NotifySecurityLock();
}

bool AHLWKeycardReader::HasControllable() const
{
	return ControlledActor && ControlledActor->Implements<UHLWAccessControllable>();
}

void AHLWKeycardReader::NotifyPower()
{
	if (HasControllable())
	{
		IHLWAccessControllable::Execute_OnReaderPowerChanged(ControlledActor, this, bPowered);
	}
}

void AHLWKeycardReader::NotifySecurityLock()
{
	if (HasControllable())
	{
		IHLWAccessControllable::Execute_OnSecurityLockChanged(ControlledActor, this, IsLockedBySystem());
	}
}

void AHLWKeycardReader::SetState(EHLWReaderState NewState)
{
	State = NewState;
	UpdateLight();
	OnStateChanged.Broadcast(State);
}

void AHLWKeycardReader::UpdateLight()
{
	FLinearColor Color = IdleColor;
	switch (State)
	{
	case EHLWReaderState::Scanning: Color = ScanningColor; break;
	case EHLWReaderState::Granted:  Color = GrantedColor;  break;
	case EHLWReaderState::Denied:   Color = DeniedColor;   break;
	case EHLWReaderState::Locked:   Color = LockedColor;   break;
	default: break;
	}

	StatusLight->SetLightColor(Color);
	StatusLight->SetIntensity(State == EHLWReaderState::Offline ? 0.f : LightIntensity);
}

void AHLWKeycardReader::PlaySound(USoundBase* Sound)
{
	if (Sound)
	{
		Audio->SetSound(Sound);
		Audio->Play();
	}
}

#undef LOCTEXT_NAMESPACE
