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

	const FGameplayAttribute Attribute = Data.EvaluatedData.Attribute;

	if (Attribute == GetInComingDamageAttribute())
	{
		// 공격 대상 설정
		const FGameplayEffectContextHandle& Context =
			Data.EffectSpec.GetEffectContext();
		AActor* InstigatorActor = Context.GetOriginalInstigator();
		ABaseMonster* Monster = Cast<ABaseMonster>(GetOwningActor());
		
		Monster->SetTargetPlayer(InstigatorActor);
		Monster->SetbIsCombat(true);
		UE_LOG(LogTemp, Warning, TEXT("Target : %s"), *InstigatorActor->GetName());


		// 데미지 적용
		float Damage = FMath::Max(0.f, GetInComingDamage() - GetDefense());
		UE_LOG(LogTemp, Warning, TEXT("%s : TakeDamage %f "), *GetName(), Damage);
		if (Damage > 0.f)
		{
			float NewHealth = FMath::Clamp(GetHealth() - Damage, 0.f, GetMaxHealth());
			SetHealth(NewHealth);
			SetInComingDamage(0.f);
		}

		// State 전환용 hit 이벤트
		UStateTreeComponent* STComp = Cast<ABaseMonster>(GetOwningActor())->GetStateTreeComponent();
		if (IsValid(STComp) == false)
		{
			return;
		}
		FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(FName("AI.Event.Hit"));
		STComp->SendStateTreeEvent(FStateTreeEvent(EventTag));
	}

	// 사망 체크
	if (GetHealth() <= 0.f) 
	{ 
		UE_LOG(LogTemp, Warning, TEXT("%s : Die "), *GetName());
		ABaseMonster* Monster = Cast<ABaseMonster>(GetOwningActor());
		if (IsValid(Monster) == false)
		{
			return;
		}
		Monster->SetbIsDead(true);

		// State 전환용 hit 이벤트
		UStateTreeComponent* STComp = Monster->GetStateTreeComponent();
		if (IsValid(STComp) == false)
		{
			return;
		}
		FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(FName("AI.Event.Die"));
		STComp->SendStateTreeEvent(FStateTreeEvent(EventTag));
	}
}

void UBaseMonsterAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

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
