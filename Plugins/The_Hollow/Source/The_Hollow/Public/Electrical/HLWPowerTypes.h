#pragma once

#include "CoreMinimal.h"
#include "HLWPowerTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHLWPower, Log, All);

// Circuit supply. Off and Faulted win over any source.
UENUM(BlueprintType)
enum class EHLWPowerState : uint8
{
	Off       UMETA(ToolTip = "Switched off at its breaker or fusebox"),
	Unpowered UMETA(ToolTip = "Switched on but no live source"),
	Backup    UMETA(ToolTip = "Fed by its backup source, emergency loads only"),
	Powered   UMETA(ToolTip = "Fed by its main source"),
	Faulted   UMETA(ToolTip = "Fault on the circuit, no power")
};

UENUM(BlueprintType)
enum class EHLWGeneratorState : uint8
{
	Off,
	Starting,
	Running,
	Failed,
	NoFuel UMETA(DisplayName = "No Fuel")
};

UENUM(BlueprintType)
enum class EHLWElectricalFaultType : uint8
{
	DamagedWiring    UMETA(DisplayName = "Damaged Wiring"),
	Overload,
	FaultedFusebox   UMETA(DisplayName = "Faulted Fusebox"),
	GeneratorFailure UMETA(DisplayName = "Generator Failure"),
	DamagedRelay     UMETA(DisplayName = "Damaged Relay"),
	WaterDamage      UMETA(DisplayName = "Water Damage")
};

// On-screen power notices, highest priority last
UENUM(BlueprintType)
enum class EHLWPowerNoticeType : uint8
{
	Restored,
	Backup,
	Failure,
	Fault
};

UENUM(BlueprintType)
enum class EHLWFuseboxState : uint8
{
	MissingFuse UMETA(DisplayName = "Missing Fuse"),
	Off,
	On,
	Faulted
};

USTRUCT(BlueprintType)
struct FHLWCircuitData
{
	GENERATED_BODY()

	// Unique across the facility, e.g. "Research.Main". Uses the actor's stable ID when empty.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power")
	FName CircuitID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power")
	FName Area;

	// Generator ID or parent circuit ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power")
	FName SourceID;

	// Used when the main source is down. Emergency loads only.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power")
	FName BackupSourceID;

	// Ignored when a breaker or fusebox controls this circuit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power")
	bool bEnabledByDefault = true;

	// Counted against generator capacity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power", meta = (ClampMin = "0"))
	float Load = 0.f;
};

USTRUCT(BlueprintType)
struct FHLWGeneratorData
{
	GENERATED_BODY()

	// Circuits use this as their SourceID. Uses the actor's stable ID when empty.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power")
	FName GeneratorID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power")
	bool bIsBackup = false;

	// Max total load. 0 = unlimited. Going over fails the generator.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power", meta = (ClampMin = "0"))
	float Capacity = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power", meta = (ClampMin = "0"))
	float StartupTime = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power")
	bool bStartsRunning = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power")
	bool bRequiresFuel = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power", meta = (ClampMin = "0", EditCondition = "bRequiresFuel"))
	float Fuel = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power", meta = (ClampMin = "0", EditCondition = "bRequiresFuel"))
	float FuelPerSecond = 1.f;

	// Stops by itself after this many seconds. 0 = runs until stopped.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power", meta = (ClampMin = "0"))
	float RunTime = 0.f;
};

USTRUCT(BlueprintType)
struct FHLWPowerConsumerData
{
	GENERATED_BODY()

	// Empty = always powered
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power")
	FName CircuitID;

	// Keeps running on backup power
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power")
	bool bEmergencyLoad = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power", meta = (ClampMin = "0"))
	float Load = 0.f;
};

USTRUCT(BlueprintType)
struct FHLWElectricalFaultData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power")
	EHLWElectricalFaultType FaultType = EHLWElectricalFaultType::DamagedWiring;

	// Circuit or generator the fault cuts
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power")
	FName TargetID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power")
	bool bActive = true;

	// Item needed to repair. Empty = interact only.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HLW|Power")
	FName RepairItem;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHLWOnCircuitStateChanged, FName, CircuitID, EHLWPowerState, State, bool, bInitial);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHLWOnGeneratorStateChanged, FName, GeneratorID, EHLWGeneratorState, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FHLWOnElectricalFault, FName, FaultID, FName, TargetID, EHLWElectricalFaultType, FaultType, bool, bActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHLWOnFuseCollected, FName, FuseType, AActor*, Collector);

namespace HLWPower
{
	inline bool IsLive(EHLWPowerState State)
	{
		return State == EHLWPowerState::Powered || State == EHLWPowerState::Backup;
	}

	// Whether a consumer runs on a circuit in this state
	inline bool SuppliesConsumer(EHLWPowerState State, bool bEmergencyLoad)
	{
		return State == EHLWPowerState::Powered || (State == EHLWPowerState::Backup && bEmergencyLoad);
	}
}
