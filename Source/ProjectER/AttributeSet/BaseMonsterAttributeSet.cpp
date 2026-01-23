#include "AttributeSet/BaseMonsterAttributeSet.h"

#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h" //FGameplayEffectModCallbackData
//#include "GameplayEffectTypes.h"  //FOnAttributeChangeData

UBaseMonsterAttributeSet::UBaseMonsterAttributeSet()
{
	InitMaxHealth(100.f);
	InitHealth(100.f);
	InitAttackPower(20.f);
	InitDefense(10.f);
	InitAttackSpeed(2.f);
	InitMoveSpeed(100.f);
	InitDamaged(0.f);
}

void UBaseMonsterAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UBaseMonsterAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseMonsterAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseMonsterAttributeSet, AttackPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseMonsterAttributeSet, Defense, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseMonsterAttributeSet, AttackSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseMonsterAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
}

void UBaseMonsterAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(1.f, NewValue);

		if (NewValue < GetHealth())
		{
			SetHealth(NewValue);
		}
	}
	else if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetAttackPowerAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
	else if (Attribute == GetDefenseAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
	else if (Attribute == GetAttackSpeedAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
	else if (Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
}

void UBaseMonsterAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	// Attribute 변경
	const FGameplayAttribute Attribute = Data.EvaluatedData.Attribute;

	if (Attribute == GetInComingDamageAttribute())
	{
		float Damage = GetInComingDamage() - GetDefense();
		if (Damage > 0.f)
		{
			float NewHealth = FMath::Clamp(GetHealth() - Damage, 0.f, GetMaxHealth());
			SetHealth(NewHealth);
			SetInComingDamage(0.f);
		}
	}

	if (GetHealth() <= 0.f) 
	{ 
		// 사망
	}
}

void UBaseMonsterAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	// Attribute 변경된 후
}


void UBaseMonsterAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseMonsterAttributeSet, MaxHealth, OldMaxHealth);
}

void UBaseMonsterAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseMonsterAttributeSet, Health, OldHealth);
}

void UBaseMonsterAttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldAttackPower)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseMonsterAttributeSet, AttackPower, OldAttackPower);
}

void UBaseMonsterAttributeSet::OnRep_Defense(const FGameplayAttributeData& OldDefense)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseMonsterAttributeSet, Defense, OldDefense);
}

void UBaseMonsterAttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldAttackSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseMonsterAttributeSet, AttackSpeed, OldAttackSpeed);
}

void UBaseMonsterAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseMonsterAttributeSet, MoveSpeed, OldMoveSpeed);
}
