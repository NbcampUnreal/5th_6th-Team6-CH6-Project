#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"
#include "BaseMonsterAttributeSet.generated.h"


//#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName)\
//GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName)\
//GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName)\
//GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName)\
//GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttributeChanged, float, OldValue, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTakeDamage, AActor*, Target);

UCLASS()
class PROJECTER_API UBaseMonsterAttributeSet : public UAttributeSet
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
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	void OnRep_AttackPower(const FGameplayAttributeData& OldAttackPower);

	UFUNCTION()
	void OnRep_Defense(const FGameplayAttributeData& OldDefense);

	UFUNCTION()
	void OnRep_AttackSpeed(const FGameplayAttributeData& OldAttackSpeed);

	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed);
#pragma endregion


public:

#pragma region Attribute
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Attribute", meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UBaseMonsterAttributeSet, MaxHealth);

	UPROPERTY(EditDefaultsOnly);
	float MaxHealthAmount = 100.f;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Attribute", meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UBaseMonsterAttributeSet, Health);

	UPROPERTY(EditDefaultsOnly);
	float HealthAmount = 100.f;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackPower, Category = "Attribute", meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS(UBaseMonsterAttributeSet, AttackPower);

	UPROPERTY(EditDefaultsOnly);
	float AttackPowerAmount = 10.f;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Defense, Category = "Attribute", meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData Defense;
	ATTRIBUTE_ACCESSORS(UBaseMonsterAttributeSet, Defense);

	UPROPERTY(EditDefaultsOnly);
	float DefenseAmount = 5.f;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackSpeed, Category = "Attribute", meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData AttackSpeed;
	ATTRIBUTE_ACCESSORS(UBaseMonsterAttributeSet, AttackSpeed);

	UPROPERTY(EditDefaultsOnly);
	float AttackSpeedAmount = 2.f;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeed, Category = "Attribute", meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UBaseMonsterAttributeSet, MoveSpeed);

	UPROPERTY(EditDefaultsOnly);
	float MoveSpeedAmount = 200.f;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute", meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData InComingDamage;
	ATTRIBUTE_ACCESSORS(UBaseMonsterAttributeSet, InComingDamage);
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
