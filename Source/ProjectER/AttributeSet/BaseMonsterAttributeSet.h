#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"
#include "BaseMonsterAttributeSet.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttributeChanged, float, OldValue, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTakeDamage, AActor*, Target);

UCLASS()
class PROJECTER_API UBaseMonsterAttributeSet : public UBaseAttributeSet
{
	GENERATED_BODY()
	
public:
	UBaseMonsterAttributeSet();


protected:

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;

	virtual void PreAttributeChange(
		const FGameplayAttribute& Attribute,
		float& NewValue
	) override;

	virtual void PostGameplayEffectExecute(
		const struct FGameplayEffectModCallbackData& Data
	) override;

	virtual void PostAttributeChange(
		const FGameplayAttribute& Attribute,
		float OldValue,
		float NewValue
	) override;


private:

#pragma region OnRep

	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth) override;


#pragma endregion


public:

#pragma region Attribute

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Monster Attribute");
	float MaxHealthAmount = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Monster Attribute");
	float HealthAmount = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Monster Attribute");
	float AttackPowerAmount = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Monster Attribute");
	float AttackSpeedAmount = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Monster Attribute");
	float AttackRangeAmount = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Monster Attribute");
	float DefenseAmount = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Monster Attribute");
	float MoveSpeedAmount = 300.f;

#pragma endregion

	UPROPERTY()
	FOnAttributeChanged OnMaxHealthChanged;

	UPROPERTY()
	FOnAttributeChanged OnHealthChanged;

	UPROPERTY()
	FOnAttributeChanged OnAttackSpeedChanged;

	UPROPERTY()
	FOnAttributeChanged OnMoveSpeedChanged;

	UPROPERTY()
	FOnTakeDamage OnMonsterHit;

	UPROPERTY()
	FOnTakeDamage OnMonsterDeath;
protected:


private:


};
