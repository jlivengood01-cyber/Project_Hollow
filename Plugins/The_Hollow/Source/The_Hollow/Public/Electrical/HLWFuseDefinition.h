#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HLWFuseDefinition.generated.h"

class UTexture2D;

// One asset per fuse type. Fuseboxes list the types they accept.
UCLASS(BlueprintType)
class THE_HOLLOW_API UHLWFuseDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Also the item ID stored on the player
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Fuse")
	FName FuseType = TEXT("Fuse.Standard");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Fuse")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HLW|Fuse")
	TObjectPtr<UTexture2D> Icon;
};
