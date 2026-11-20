// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HLWHealthComponent.generated.h"

class UDamageType;
class AController;

UENUM(BlueprintType)
enum class EHLWHealthState : uint8
{
	Healthy,
	Injured,
	Dead
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHLWOnHealthChangedSignature, float, CurrentHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHLWOnHealthStateChangedSignature, EHLWHealthState, NewState, EHLWHealthState, OldState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHLWOnDamageReceivedSignature, float, Damage, const UDamageType*, DamageType, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHLWOnHealingReceivedSignature, float, HealedAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHLWOnDeathSignature, AActor*, DamageCauser);

/**
 * Tracks health and a derived Healthy/Injured/Dead state.
 * Receives damage through Unreal's ApplyDamage (owner's OnTakeAnyDamage).
 * Only broadcasts events: UI, movement, death handling etc. react to them.
 */
UCLASS(ClassGroup = (HLW), meta = (BlueprintSpawnableComponent))
class THE_HOLLOW_API UHLWHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHLWHealthComponent();

	/** Returns true if any healing was applied. */
	UFUNCTION(BlueprintCallable, Category = "HLW|Health")
	bool ApplyHealing(float Amount);

	UFUNCTION(BlueprintCallable, Category = "HLW|Health")
	void ResetHealth();

	UFUNCTION(BlueprintCallable, Category = "HLW|Health")
	void SetInvulnerable(bool bNewInvulnerable) { bInvulnerable = bNewInvulnerable; }

	UFUNCTION(BlueprintPure, Category = "HLW|Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "HLW|Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "HLW|Health")
	float GetHealthPercent() const { return MaxHealth > 0.f ? CurrentHealth / MaxHealth : 0.f; }

	UFUNCTION(BlueprintPure, Category = "HLW|Health")
	EHLWHealthState GetHealthState() const { return HealthState; }

	UFUNCTION(BlueprintPure, Category = "HLW|Health")
	bool IsDead() const { return HealthState == EHLWHealthState::Dead; }

	UFUNCTION(BlueprintPure, Category = "HLW|Health")
	bool IsInvulnerable() const { return bInvulnerable; }

	UPROPERTY(BlueprintAssignable, Category = "HLW|Health")
	FHLWOnHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "HLW|Health")
	FHLWOnHealthStateChangedSignature OnHealthStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "HLW|Health")
	FHLWOnDamageReceivedSignature OnDamageReceived;

	UPROPERTY(BlueprintAssignable, Category = "HLW|Health")
	FHLWOnHealingReceivedSignature OnHealingReceived;

	UPROPERTY(BlueprintAssignable, Category = "HLW|Health")
	FHLWOnDeathSignature OnDeath;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "HLW|Health", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

	/** At or below this fraction of MaxHealth the state becomes Injured. */
	UPROPERTY(EditDefaultsOnly, Category = "HLW|Health", meta = (ClampMin = "0", ClampMax = "1"))
	float InjuredThresholdPercent = 0.5f;

private:
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		AController* InstigatedBy, AActor* DamageCauser);

	void UpdateHealthState(AActor* DamageCauser);

	UPROPERTY(VisibleInstanceOnly, Category = "HLW|Health")
	float CurrentHealth = 0.f;

	UPROPERTY(VisibleInstanceOnly, Category = "HLW|Health")
	EHLWHealthState HealthState = EHLWHealthState::Healthy;

	UPROPERTY(VisibleInstanceOnly, Category = "HLW|Health")
	bool bInvulnerable = false;

	void DebugShowStatus() const;
};
