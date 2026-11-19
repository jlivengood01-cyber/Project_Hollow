#include "Access/HLWSecurityTerminal.h"

#include "Access/HLWFacilityAccessSubsystem.h"
#include "Access/HLWKeycardInventoryComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "HLWSecurityTerminal"

AHLWSecurityTerminal::AHLWSecurityTerminal()
{
	Mesh->SetRelativeScale3D(FVector(0.4f, 0.6f, 0.4f));

	AccessData.DeviceType = EHLWAccessDeviceType::Terminal;
	AccessData.bRequiresNetwork = true;
}

void AHLWSecurityTerminal::BeginPlay()
{
	Super::BeginPlay();

	if (AccessData.DeviceID.IsNone())
	{
		AccessData.DeviceID = HLWAccess::MakeStableID(this);
	}
}

bool AHLWSecurityTerminal::IsAvailable() const
{
	return (!AccessData.bRequiresPower || bPowered)
		&& (!AccessData.bRequiresNetwork || bNetworked)
		&& bOperational;
}

void AHLWSecurityTerminal::OnDeviceStateChanged(bool bPowerChanged)
{
	// Losing power or network ends the session
	if (bLoggedIn && !IsAvailable())
	{
		LogOut();
	}
}

EHLWAccessResult AHLWSecurityTerminal::LogIn(AActor* User)
{
	UHLWFacilityAccessSubsystem* Access = GetWorld()->GetSubsystem<UHLWFacilityAccessSubsystem>();
	if (!Access)
	{
		return EHLWAccessResult::InvalidDevice;
	}

	const UHLWKeycardInventoryComponent* Inventory = User ? User->FindComponentByClass<UHLWKeycardInventoryComponent>() : nullptr;
	const EHLWAccessResult Result = Access->RequestAccess(Inventory, AccessData, this);
	ShowDebug(TEXT("Log in"), Result);

	if (Result == EHLWAccessResult::Granted && !bLoggedIn)
	{
		bLoggedIn = true;
		LastFunctionIndex = INDEX_NONE;
		OnLoggedIn.Broadcast(User);
	}
	return Result;
}

void AHLWSecurityTerminal::LogOut()
{
	if (bLoggedIn)
	{
		bLoggedIn = false;
		LastFunctionIndex = INDEX_NONE;
		OnLoggedOut.Broadcast();
	}
}

EHLWAccessResult AHLWSecurityTerminal::UseFunction(FName FunctionID, AActor* User)
{
	const FHLWTerminalFunction* Function = Functions.FindByPredicate([FunctionID](const FHLWTerminalFunction& Entry)
	{
		return Entry.FunctionID == FunctionID;
	});
	if (!Function)
	{
		return EHLWAccessResult::InvalidDevice;
	}

	if (!bLoggedIn)
	{
		const EHLWAccessResult LoginResult = LogIn(User);
		if (LoginResult != EHLWAccessResult::Granted)
		{
			OnFunctionDenied.Broadcast(FunctionID, LoginResult);
			return LoginResult;
		}
	}

	UHLWFacilityAccessSubsystem* Access = GetWorld()->GetSubsystem<UHLWFacilityAccessSubsystem>();
	if (!Access)
	{
		return EHLWAccessResult::InvalidDevice;
	}

	// Re-run the full check so a lockdown or outage mid-session still applies
	FHLWFacilityAccessData FunctionData = AccessData;
	FunctionData.DeviceID = FName(*FString::Printf(TEXT("%s.%s"), *AccessData.DeviceID.ToString(), *FunctionID.ToString()));
	if (Function->bRequiresPermission)
	{
		FunctionData.Requirement = Function->Permission;
	}

	const UHLWKeycardInventoryComponent* Inventory = User ? User->FindComponentByClass<UHLWKeycardInventoryComponent>() : nullptr;
	const EHLWAccessResult Result = Access->RequestAccess(Inventory, FunctionData, this);
	ShowDebug(Function->DisplayName.ToString(), Result);

	if (Result == EHLWAccessResult::Granted)
	{
		OnFunctionAuthorized.Broadcast(FunctionID, User);
	}
	else
	{
		OnFunctionDenied.Broadcast(FunctionID, Result);
	}
	return Result;
}

void AHLWSecurityTerminal::Interact_Implementation(AActor* InteractingActor)
{
	if (!bLoggedIn)
	{
		LogIn(InteractingActor);
		return;
	}

	const int32 Next = LastFunctionIndex + 1;
	if (!Functions.IsValidIndex(Next))
	{
		LogOut();
		return;
	}

	LastFunctionIndex = Next;
	UseFunction(Functions[Next].FunctionID, InteractingActor);
}

FText AHLWSecurityTerminal::GetInteractionText_Implementation()
{
	if (!bLoggedIn)
	{
		return LOCTEXT("LogIn", "Log in to terminal");
	}

	const int32 Next = LastFunctionIndex + 1;
	return Functions.IsValidIndex(Next)
		? FText::Format(LOCTEXT("Run", "Run: {0}"), Functions[Next].DisplayName)
		: LOCTEXT("LogOut", "Log out");
}

void AHLWSecurityTerminal::ShowDebug(const FString& Label, EHLWAccessResult Result) const
{
	if (bShowDebugResult && GEngine)
	{
		const FColor Color = Result == EHLWAccessResult::Granted ? FColor::Green : FColor::Red;
		GEngine->AddOnScreenDebugMessage(-1, 3.f, Color, FString::Printf(TEXT("Terminal %s: %s"), *Label, *UEnum::GetDisplayValueAsText(Result).ToString()));
	}
}

#undef LOCTEXT_NAMESPACE
