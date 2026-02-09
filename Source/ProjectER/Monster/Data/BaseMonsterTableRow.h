#pragma once

#include "CoreMinimal.h"
#include "BaseMonsterTableRow.generated.h"

USTRUCT(BlueprintType)
struct PROJECTER_API FBaseMonsterTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	float BaseMaxLevel;

	UPROPERTY(EditAnywhere)
	float BaseMaxXP;

	UPROPERTY(EditAnywhere)
	float BaseMaxHealth;

	UPROPERTY(EditAnywhere)
	float BaseHealthRegen;

	UPROPERTY(EditAnywhere)
	float BaseMaxStamina;

	UPROPERTY(EditAnywhere)
	float BaseStaminaRegen;

	UPROPERTY(EditAnywhere)
	float BaseAttackPower;

	UPROPERTY(EditAnywhere)
	float BaseAttackSpeed;

	UPROPERTY(EditAnywhere)
	float BaseAttackRange;

	UPROPERTY(EditAnywhere)
	float BaseSkillAmp;

	UPROPERTY(EditAnywhere)
	float BaseCriticalChance;

	UPROPERTY(EditAnywhere)
	float BaseCriticalDamage;

	UPROPERTY(EditAnywhere)
	float BaseDefense;

	UPROPERTY(EditAnywhere)
	float BaseMoveSpeed;

	UPROPERTY(EditAnywhere)
	float BaseCooldownReduction;

	UPROPERTY(EditAnywhere)
	float BaseTenacity;
};
