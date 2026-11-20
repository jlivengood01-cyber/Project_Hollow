#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Access/HLWAccessTypes.h"
#include "HLWAccessMessageSet.generated.h"

// Player-facing wording for each access result. Missing entries use built-in defaults.
UCLASS(BlueprintType)
class THE_HOLLOW_API UHLWAccessMessageSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HLW|Access")
	TMap<EHLWAccessResult, FText> Messages;

	UFUNCTION(BlueprintPure, Category = "HLW|Access")
	FText GetMessage(EHLWAccessResult Result) const;

	static FText GetDefaultMessage(EHLWAccessResult Result);
};
