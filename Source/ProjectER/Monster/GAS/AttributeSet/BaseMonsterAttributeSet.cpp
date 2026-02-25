#include "Monster/GAS/AttributeSet/BaseMonsterAttributeSet.h"

#include "Monster/BaseMonster.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Components/StateTreeComponent.h"

UBaseMonsterAttributeSet::UBaseMonsterAttributeSet()
{

}

float UBaseMonsterAttributeSet::GetHPPersent()
{
	float Persent = FMath::Clamp(GetHealth() / GetMaxHealth(), 0.f, 100.f);
	return Persent;
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
		
		//AActor* Target = Context.GetInstigator(); // 인스티게이터로
		AActor* Target = Context.GetEffectCauser(); 
		ABaseMonster* Monster = Cast<ABaseMonster>(GetOwningActor());
		if (IsValid(Target) == false)
		{
			UE_LOG(LogTemp, Warning, TEXT("UBaseMonsterAttributeSet::PostGameplayEffectExecute : Not Target"));
			return;
		}

		if (GetHealth() <= 0.f)
		{
			OnMonsterDeath.Broadcast(Target);
		}
		else
		{
			OnMonsterHit.Broadcast(Target);
		}
	}
}

void UBaseMonsterAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

}

void UBaseMonsterAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	OnHealthChanged.Broadcast(GetHealth(), GetMaxHealth());
}

void UBaseMonsterAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldHealth)
{
	OnMoveSpeedChanged.Broadcast(OldHealth.GetBaseValue(), GetMoveSpeed());
}
