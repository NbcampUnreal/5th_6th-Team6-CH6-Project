#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BaseItemData.generated.h"

/**
 * 아이템의 정적 데이터를 관리하는 베이스 데이터 에셋
 */
UCLASS(BlueprintType)
class PROJECTER_API UBaseItemData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 생성자 선언
	UBaseItemData();

	// 에셋 매니저 식별자
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Info")
	FText ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visual")
	TSoftObjectPtr<UStaticMesh> ItemMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visual")
	TSoftObjectPtr<UTexture2D> ItemIcon;
};