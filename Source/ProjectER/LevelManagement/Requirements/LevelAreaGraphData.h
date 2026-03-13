#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LevelAreaGraphData.generated.h"

USTRUCT(BlueprintType)
struct FLevelAreaNodeRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area")
	int32 NodeID = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area")
	TArray<int32> NeighborIDs;
};

UCLASS()
class PROJECTER_API ULevelAreaGraphData : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area")
	TArray<FLevelAreaNodeRecord> Nodes;
};