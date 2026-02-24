#pragma once

#include "CoreMinimal.h"
#include "BaseMonsterTableRow.generated.h"

USTRUCT(BlueprintType)
struct PROJECTER_API FBaseMonsterTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	float BaseMaxLevel = 0;

	UPROPERTY(EditAnywhere)
	float BaseMaxXP = 0;

	UPROPERTY(EditAnywhere)
	float BaseMaxHealth = 0;

	UPROPERTY(EditAnywhere)
	float BaseHealthRegen = 0;

	UPROPERTY(EditAnywhere)
	float BaseMaxStamina = 0;

	UPROPERTY(EditAnywhere)
	float BaseStaminaRegen = 0;

	UPROPERTY(EditAnywhere)
	float BaseAttackPower = 0;

	UPROPERTY(EditAnywhere)
	float BaseAttackSpeed = 0;

	UPROPERTY(EditAnywhere)
	float BaseAttackRange = 0;

	UPROPERTY(EditAnywhere)
	float BaseSkillAmp = 0;

	UPROPERTY(EditAnywhere)
	float BaseCriticalChance = 0;

	UPROPERTY(EditAnywhere)
	float BaseCriticalDamage = 0;

	UPROPERTY(EditAnywhere)
	float BaseDefense = 0;

	UPROPERTY(EditAnywhere)
	float BaseMoveSpeed = 0;

	UPROPERTY(EditAnywhere)
	float BaseCooldownReduction = 0;

	UPROPERTY(EditAnywhere)
	float BaseTenacity = 0;

};
