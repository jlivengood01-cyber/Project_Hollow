#include "Electrical/HLWPoweredActor.h"

#include "Electrical/HLWPowerConsumerComponent.h"

AHLWPoweredActor::AHLWPoweredActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Power = CreateDefaultSubobject<UHLWPowerConsumerComponent>(TEXT("Power"));
}

bool AHLWPoweredActor::IsPowered() const
{
	return Power && Power->IsPowered();
}

void AHLWPoweredActor::BeginPlay()
{
	Super::BeginPlay();

	Power->OnPowerChanged.AddDynamic(this, &ThisClass::HandlePowerChanged);
	Power->OnCircuitStateChanged.AddDynamic(this, &ThisClass::HandleCircuitStateChanged);

	// The consumer already has its state by now
	ApplyPower(Power->IsPowered(), true);
	ApplyCircuitState(Power->GetCircuitState(), true);
}

void AHLWPoweredActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	Super::EndPlay(EndPlayReason);
}

void AHLWPoweredActor::HandlePowerChanged(bool bPowered, bool bInitial)
{
	ApplyPower(bPowered, bInitial);
}

void AHLWPoweredActor::HandleCircuitStateChanged(EHLWPowerState State, bool bInitial)
{
	ApplyCircuitState(State, bInitial);
}
