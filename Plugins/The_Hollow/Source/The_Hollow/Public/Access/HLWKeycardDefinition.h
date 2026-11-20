#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Access/HLWAccessTypes.h"
#include "HLWKeycardDefinition.generated.h"

class UTexture2D;

// One asset per keycard
UCLASS(BlueprintType)
class THE_HOLLOW_API UHLWKeycardDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Unique and stable, used by saves and access rules
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HLW|Keycard")
	FName KeycardID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HLW|Keycard")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HLW|Keycard", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HLW|Keycard")
	EHLWKeycardAccessLevel AccessLevel = EHLWKeycardAccessLevel::General;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HLW|Keycard")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HLW|Keycard")
	TObjectPtr<UTexture2D> Icon;

	// Unique cards can only be held once
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HLW|Keycard")
	bool bUnique = true;
};
