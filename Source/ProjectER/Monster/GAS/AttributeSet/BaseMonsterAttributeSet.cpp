#include "Monster/GAS/AttributeSet/BaseMonsterAttributeSet.h"

#include "Monster/BaseMonster.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Components/StateTreeComponent.h"

UBaseMonsterAttributeSet::UBaseMonsterAttributeSet()
{
	InitHealth(HealthAmount);
	InitMaxHealth(MaxHealthAmount);
	InitAttackPower(AttackPowerAmount);
	InitAttackSpeed(AttackSpeedAmount);
	InitAttackRange(AttackRangeAmount);
	InitDefense(DefenseAmount);
	InitMoveSpeed(MoveSpeedAmount);
}

void UBaseMonsterAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

}

void UBaseMonsterAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

}

void UBaseMonsterAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	//GameplayEffect가 Execute로 속성을 변경했을 때
	const FGameplayAttribute Attribute = Data.EvaluatedData.Attribute;

	if (Attribute == GetIncomingDamageAttribute())
	{
		// 공격 대상 설정
		const FGameplayEffectContextHandle& Context =
			Data.EffectSpec.GetEffectContext();

		AActor* Target = Context.GetEffectCauser();
		ABaseMonster* Monster = Cast<ABaseMonster>(GetOwningActor());
		
		OnMonsterHit.Broadcast(Target);
		
		if (GetHealth() <= 0.f)
		{
			OnMonsterDeath.Broadcast(Target);
		}
	}
}

void UBaseMonsterAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// Attribute 변경된 후
	/*if (Attribute == GetMaxHealthAttribute())
	{
		OnMaxHealthChanged.Broadcast(OldValue, NewValue);
	}
	else if (Attribute == GetHealthAttribute())
	{
		OnHealthChanged.Broadcast(OldValue, NewValue);
	}
	else if (Attribute == GetAttackSpeedAttribute())
	{
		OnAttackSpeedChanged.Broadcast(OldValue, NewValue);
	}
	else if (Attribute == GetMoveSpeedAttribute())
	{
		OnMoveSpeedChanged.Broadcast(OldValue, NewValue);
	}*/
}

void UBaseMonsterAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	OnHealthChanged.Broadcast(GetHealth(), GetMaxHealth());
}

