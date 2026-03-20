#pragma once

#include "CoreMinimal.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "UsableItemData.generated.h"

class UGameplayEffect;

UENUM(BlueprintType)
enum class EItemEffectType : uint8
{
	None            UMETA(DisplayName = "None"),
	IncreaseStat    UMETA(DisplayName = "Increase Stat"),
	HealOverTime    UMETA(DisplayName = "Heal Over Time"),
	ManaOverTime    UMETA(DisplayName = "Mana Over Time")
};

UENUM(BlueprintType)
enum class EItemStatType : uint8
{
	AttackPower     UMETA(DisplayName = "Attack Power"),
	Defense         UMETA(DisplayName = "Defense"),
	AttackSpeed     UMETA(DisplayName = "Attack Speed"),
	MoveSpeed       UMETA(DisplayName = "Move Speed")
};

UCLASS(BlueprintType)
class PROJECTER_API UUsableItemData : public UBaseItemData
{
	GENERATED_BODY()

public:
	UUsableItemData();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Effect")
	EItemEffectType EffectType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Effect", meta = (EditCondition = "EffectType == EItemEffectType::IncreaseStat", EditConditionHides))
	EItemStatType StatType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Effect", meta = (EditCondition = "EffectType == EItemEffectType::IncreaseStat", EditConditionHides))
	float EffectValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Effect", meta = (EditCondition = "EffectType == EItemEffectType::HealOverTime", EditConditionHides, ClampMin = "0.0"))
	float TotalHealAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Effect", meta = (EditCondition = "EffectType == EItemEffectType::HealOverTime", EditConditionHides, ClampMin = "0.1"))
	float HealDurationSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Effect", meta = (EditCondition = "EffectType == EItemEffectType::HealOverTime", EditConditionHides, ClampMin = "0.1"))
	float HealTickInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Effect", meta = (EditCondition = "EffectType == EItemEffectType::ManaOverTime", EditConditionHides, ClampMin = "0.0"))
	float TotalManaAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Effect", meta = (EditCondition = "EffectType == EItemEffectType::ManaOverTime", EditConditionHides, ClampMin = "0.1"))
	float ManaDurationSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Effect", meta = (EditCondition = "EffectType == EItemEffectType::ManaOverTime", EditConditionHides, ClampMin = "0.1"))
	float ManaTickInterval;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Effect", meta = (EditCondition = "EffectType == EItemEffectType::IncreaseStat", EditConditionHides))
	TSoftClassPtr<UGameplayEffect> ItemStatEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Settings")
	bool bConsumable;
};