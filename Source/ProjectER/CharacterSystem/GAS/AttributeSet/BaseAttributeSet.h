
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "BaseAttributeSet.generated.h"

// ===== Attribute 매크로 =====
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class PROJECTER_API UBaseAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
	public:
	UBaseAttributeSet();
	
	// 네트워크 복제
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	// Attribute 값 변경 전 처리
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	// GameplayEffect 적용 후 처리
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	
#pragma region Vital Attribute
	// XP (경험치 = HP)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Vital", ReplicatedUsing = OnRep_XP)
	FGameplayAttributeData XP;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, XP);
	
	// MaxXP (최대 경험치)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Vital", ReplicatedUsing = OnRep_MaxXP)
	FGameplayAttributeData MaxXP;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MaxXP);
	
	// Health (체력 = HP)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Vital", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Health);

	// MaxHealth (최대 체력)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Vital", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MaxHealth);
	
	// HealthRegen (체력 재생)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Vital", ReplicatedUsing = OnRep_HealthRegen)
	FGameplayAttributeData HealthRegen;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, HealthRegen);

	// Stamina (스테미나 = MP)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Vital", ReplicatedUsing = OnRep_Stamina)
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Stamina);
	
	// MaxStamina (최대 스테미나)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Vital", ReplicatedUsing = OnRep_MaxStamina)
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MaxStamina);
	
	// StaminaRegen (스테미나 재생)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Vital", ReplicatedUsing = OnRep_StaminaRegen)
	FGameplayAttributeData StaminaRegen;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, StaminaRegen);
	
#pragma endregion 
	
#pragma region Combat Attribute
	// Attack Power (공격력)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat", ReplicatedUsing = OnRep_AttackPower)
	FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, AttackPower);
	
	// Attack Speed (공격 속도)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat", ReplicatedUsing = OnRep_AttackSpeed)
	FGameplayAttributeData AttackSpeed;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, AttackSpeed);
	
	// Skill Amp (스킬 증폭)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat", ReplicatedUsing = OnRep_SkillAmp)
	FGameplayAttributeData SkillAmp;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, SkillAmp)
	
	// Critical Chance (치명타 확률)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat", ReplicatedUsing = OnRep_CriticalChance)
	FGameplayAttributeData CriticalChance;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, CriticalChance)
	
	// Critical Damage (치명타 피해량)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat", ReplicatedUsing = OnRep_CriticalDamage)
	FGameplayAttributeData CriticalDamage;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, CriticalDamage)
	
	// Defense (방어력)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat", ReplicatedUsing = OnRep_Defense)
	FGameplayAttributeData Defense;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Defense);
	
	// Move Speed (이동 속도)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat", ReplicatedUsing = OnRep_MoveSpeed)
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MoveSpeed)
	
	// Cooldown Reduction (쿨타임 감소)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat", ReplicatedUsing = OnRep_CooldownReduction)
	FGameplayAttributeData CooldownReduction;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, CooldownReduction)
	
	// Tenacity (강인함 - CC저항)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat", ReplicatedUsing = OnRep_Tenacity)
	FGameplayAttributeData Tenacity;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Tenacity)
	
#pragma endregion
	
#pragma region Meta Attribute
	// Incoming Damage (데미지 계산용) 
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, IncomingDamage);
	
	// Incoming XP (경험치 계산용)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Meta")
	FGameplayAttributeData IncomingXP;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, IncomingXP);
	
#pragma endregion
	
protected:
	// OnRep Functions
	UFUNCTION() 
	void OnRep_XP(const FGameplayAttributeData& OldXP);
	UFUNCTION() 
	void OnRep_MaxXP(const FGameplayAttributeData& OldMaxXP);
	UFUNCTION() 
	void OnRep_Health(const FGameplayAttributeData& OldHealth);
	UFUNCTION() 
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);
	UFUNCTION() 
	void OnRep_HealthRegen(const FGameplayAttributeData& OldHealthRegen);
	UFUNCTION() 
	void OnRep_Stamina(const FGameplayAttributeData& OldStamina);
	UFUNCTION() 
	void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina);
	UFUNCTION() 
	void OnRep_StaminaRegen(const FGameplayAttributeData& OldStaminaRegen);
	UFUNCTION() 
	void OnRep_AttackPower(const FGameplayAttributeData& OldAttackPower);
	UFUNCTION() 
	void OnRep_AttackSpeed(const FGameplayAttributeData& OldAttackSpeed);
	UFUNCTION() 
	void OnRep_SkillAmp(const FGameplayAttributeData& OldSkillAmp);
	UFUNCTION() 
	void OnRep_CriticalChance(const FGameplayAttributeData& OldCriticalChance);
	UFUNCTION() 
	void OnRep_CriticalDamage(const FGameplayAttributeData& OldCriticalDamage);
	UFUNCTION() 
	void OnRep_Defense(const FGameplayAttributeData& OldDefense);
	UFUNCTION() 
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed);
	UFUNCTION() 
	void OnRep_CooldownReduction(const FGameplayAttributeData& OldCooldownReduction);
	UFUNCTION() 
	void OnRep_Tenacity(const FGameplayAttributeData& OldTenacity);
};
