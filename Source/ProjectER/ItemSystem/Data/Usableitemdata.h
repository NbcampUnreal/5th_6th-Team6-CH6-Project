#pragma once

#include "CoreMinimal.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "UsableItemData.generated.h"

// 아이템 효과 타입
UENUM(BlueprintType)
enum class EItemEffectType : uint8
{
	None            UMETA(DisplayName = "None"),
	IncreaseStat    UMETA(DisplayName = "Increase Stat")   // 스탯 증가
};

// EStatType → EItemStatType (충돌 방지)
UENUM(BlueprintType)
enum class EItemStatType : uint8
{
	AttackPower     UMETA(DisplayName = "Attack Power"),   // 공격력
	Defense         UMETA(DisplayName = "Defense"),        // 방어력
	AttackSpeed     UMETA(DisplayName = "Attack Speed"),   // 공격 속도
	MoveSpeed       UMETA(DisplayName = "Move Speed")      // 이동 속도
};

/**
 * 사용 가능한 아이템 데이터
 * 공격력 강화, 방어력 강화 등
 */
UCLASS(BlueprintType)
class PROJECTER_API UUsableItemData : public UBaseItemData
{
	GENERATED_BODY()

public:
	UUsableItemData();

	// 아이템 효과 타입
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Effect")
	EItemEffectType EffectType;

	// EStatType → EItemStatType
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Effect", meta = (EditCondition = "EffectType == EItemEffectType::IncreaseStat", EditConditionHides))
	EItemStatType StatType;

	// 증가량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Effect")
	float EffectValue;

	// 소비 아이템 여부 (사용 후 삭제)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Settings")
	bool bConsumable;
};