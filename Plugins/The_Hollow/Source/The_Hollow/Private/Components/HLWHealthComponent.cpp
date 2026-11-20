// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/HLWHealthComponent.h"
#include "Debug/HLWDebugLibrary.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogHLWHealth, Log, All);

UHLWHealthComponent::UHLWHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHLWHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	HealthState = EHLWHealthState::Healthy;

	if (AActor* OwningActor = GetOwner())
	{
		OwningActor->OnTakeAnyDamage.AddDynamic(this, &ThisClass::HandleTakeAnyDamage);
	}

	DebugShowStatus(); // DEBUG
}

void UHLWHealthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AActor* OwningActor = GetOwner())
	{
		OwningActor->OnTakeAnyDamage.RemoveDynamic(this, &ThisClass::HandleTakeAnyDamage);
	}

	Super::EndPlay(EndPlayReason);
}

void UHLWHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.f || IsDead() || bInvulnerable)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.f, MaxHealth);

	UE_LOG(LogHLWHealth, Log, TEXT("%s took %.1f damage from %s (%.1f/%.1f)"),
		*GetNameSafe(GetOwner()), Damage, *GetNameSafe(DamageCauser), CurrentHealth, MaxHealth);

	UHLWDebugLibrary::HLWDebugMessage(FString::Printf(TEXT("Took %.0f damage from %s"),
		Damage, *GetNameSafe(DamageCauser)), FLinearColor::Red); // DEBUG

	OnDamageReceived.Broadcast(Damage, DamageType, DamageCauser);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	UpdateHealthState(DamageCauser);
}

bool UHLWHealthComponent::ApplyHealing(float Amount)
{
	if (Amount <= 0.f || IsDead() || CurrentHealth >= MaxHealth)
	{
		return false;
	}

	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Min(CurrentHealth + Amount, MaxHealth);

	OnHealingReceived.Broadcast(CurrentHealth - PreviousHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	UHLWDebugLibrary::HLWDebugMessage(FString::Printf(TEXT("Healed %.0f"),
		CurrentHealth - PreviousHealth), FLinearColor::Green); // DEBUG

	UpdateHealthState(nullptr);
	return true;
}

void UHLWHealthComponent::ResetHealth()
{
	CurrentHealth = MaxHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	UpdateHealthState(nullptr);
}

void UHLWHealthComponent::UpdateHealthState(AActor* DamageCauser)
{
	EHLWHealthState NewState = EHLWHealthState::Healthy;
	if (CurrentHealth <= 0.f)
	{
		NewState = EHLWHealthState::Dead;
	}
	else if (CurrentHealth <= MaxHealth * InjuredThresholdPercent)
	{
		NewState = EHLWHealthState::Injured;
	}

	if (NewState == HealthState)
	{
		return;
	}

	const EHLWHealthState OldState = HealthState;
	HealthState = NewState;

	UE_LOG(LogHLWHealth, Log, TEXT("%s health state: %s -> %s"),
		*GetNameSafe(GetOwner()), *UEnum::GetValueAsString(OldState), *UEnum::GetValueAsString(NewState));

	UHLWDebugLibrary::HLWDebugMessage(
		FString::Printf(TEXT("Status: %s -> %s"),
			*UEnum::GetDisplayValueAsText(OldState).ToString(),
			*UEnum::GetDisplayValueAsText(NewState).ToString()),
		FLinearColor(1.f, 0.5f, 0.f), 5.f); // DEBUG

	OnHealthStateChanged.Broadcast(NewState, OldState);

	if (NewState == EHLWHealthState::Dead)
	{
		OnDeath.Broadcast(DamageCauser); // State only changes once, so this fires once

		UHLWDebugLibrary::HLWDebugMessage(TEXT("PLAYER DIED"), FLinearColor::Red, 6.f); // DEBUG
	}
}

void UHLWHealthComponent::DebugShowStatus() const
{
	UHLWDebugLibrary::HLWDebugMessage(
		FString::Printf(TEXT("%s Health: %.0f / %.0f  [%s]"), *GetNameSafe(GetOwner()), CurrentHealth, MaxHealth,
			*UEnum::GetDisplayValueAsText(HealthState).ToString()),
		FLinearColor::White, 30.f, TEXT("HLW.Health.Status"));
}