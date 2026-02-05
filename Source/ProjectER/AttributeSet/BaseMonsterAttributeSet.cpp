#include "AttributeSet/BaseMonsterAttributeSet.h"

#include "Monster/BaseMonster.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Components/StateTreeComponent.h"

UBaseMonsterAttributeSet::UBaseMonsterAttributeSet()
{
	InitMaxHealth(MaxHealthAmount);
	InitHealth(HealthAmount);
	InitAttackPower(AttackPowerAmount);
	InitDefense(DefenseAmount);
	InitAttackSpeed(AttackSpeedAmount);
	InitMoveSpeed(MoveSpeedAmount);
	InitInComingDamage(0.f);
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
	Super::PreAttributeBaseChange(Attribute, NewValue);

	//GameplayEffect가 적용되기 직전
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
	Super::PostGameplayEffectExecute(Data);

	//GameplayEffect가 Execute로 속성을 변경했을 때
	const FGameplayAttribute Attribute = Data.EvaluatedData.Attribute;

	if (Attribute == GetInComingDamageAttribute())
	{
		// 공격 대상 설정
		const FGameplayEffectContextHandle& Context =
			Data.EffectSpec.GetEffectContext();
		AActor* InstigatorActor = Context.GetOriginalInstigator();
		ABaseMonster* Monster = Cast<ABaseMonster>(GetOwningActor());

		// 데미지 계산 임시
		float Damage = FMath::Max(0.f, GetInComingDamage() - GetDefense());
		SetInComingDamage(0.f);

		float NewHealth = FMath::Clamp(GetHealth() - Damage, 0.f, GetMaxHealth());
		SetHealth(NewHealth);
		UE_LOG(LogTemp, Warning, TEXT("%s : Hp %f / %f "), *Monster->GetName(), GetHealth(), GetMaxHealth());
		
		OnMonsterHit.Broadcast(InstigatorActor);
		
		if (GetHealth() <= 0.f)
		{
			OnMonsterDeath.Broadcast(InstigatorActor);
		}
	}
}

void UBaseMonsterAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// Attribute 변경된 후
	if (Attribute == GetMaxHealthAttribute())
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
	}
}


void UBaseMonsterAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseMonsterAttributeSet, MaxHealth, OldMaxHealth);
}

void UBaseMonsterAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseMonsterAttributeSet, Health, OldHealth);

	OnHealthChanged.Broadcast(GetHealth(), GetMaxHealth());
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
