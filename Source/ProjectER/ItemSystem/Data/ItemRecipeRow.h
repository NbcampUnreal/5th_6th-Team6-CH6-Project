// ItemRecipeRow.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ItemRecipeRow.generated.h"

class UBaseItemData;
class USoundBase;

/**
 * 아이템 조합 레시피 구조체
 */
USTRUCT(BlueprintType)
struct FItemRecipeRow : public FTableRowBase
{
	GENERATED_BODY()

	// 재료 1
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	TSoftObjectPtr<UBaseItemData> Material1;

	// 재료 2
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	TSoftObjectPtr<UBaseItemData> Material2;

	// 결과 아이템
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	TSoftObjectPtr<UBaseItemData> ResultItem;

	// 조합 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe", meta = (ClampMin = "0.1"))
	float CraftTime;

	// 조합 사운드
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	TSoftObjectPtr<USoundBase> CraftSound;

	// 우선순위 (높을수록 먼저 선택)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	int32 Priority;

	FItemRecipeRow()
		: CraftTime(4.0f)
		, Priority(0)
	{}
};