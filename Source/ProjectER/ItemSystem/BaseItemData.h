#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BaseItemData.generated.h"

UENUM(BlueprintType)
enum class EItemPickupType : uint8
{
    Automatic    UMETA(DisplayName = "Automatic Pickup (Overlap)"),
    Interaction  UMETA(DisplayName = "Manual Pickup (Click)")
};

UCLASS(BlueprintType)
class PROJECTER_API UBaseItemData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UBaseItemData();

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Info")
    FText ItemName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Visual")
    TSoftObjectPtr<UStaticMesh> ItemMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Visual")
    TSoftObjectPtr<UTexture2D> ItemIcon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Settings")
    EItemPickupType PickupType;
};