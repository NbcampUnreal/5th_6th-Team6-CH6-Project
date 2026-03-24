#include "Monster/GAS/GA/GA_MonsterState_Attack.h"
#include "Monster/BaseMonster.h"
#include "Monster/GAS/AttributeSet/BaseMonsterAttributeSet.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

UGA_MonsterState_Attack::UGA_MonsterState_Attack()
{
	StateInitData.MonsterAssetTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability.Action.AutoAttack"));
	StateInitData.MontageType = EMonsterMontageType::Attack;
	StateInitData.NiagaraCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Particle.Skill.AutoAttack");
	StateInitData.SoundCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Sound.Skill.AutoAttack");
	StateInitData.WaitTag = FGameplayTag::RequestGameplayTag("State.Action.Attack");

	SetAssetTags(StateInitData.MonsterAssetTags);
}

void UGA_MonsterState_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ABaseMonster* Monster = Cast<ABaseMonster>(GetOwningActorFromActorInfo());
	if (IsValid(Monster))
	{
		AActor* Target = Monster->GetTargetPlayer();
		if (IsValid(Target))
		{
			UBaseAttributeSet* AS = Monster->GetAttributeSet();
			if (AS)
			{
				bool bIsDead = false;
				if (AS->GetHealth() <= 0.0f)
				{
					bIsDead = true;
				}

				if (bIsDead)
				{
					Monster->SetTargetPlayer(nullptr);
					EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
					return;
				}
			}

			FVector Direction = Target->GetActorLocation() - Monster->GetActorLocation();
			Direction.Z = 0.0f;
			Monster->SetActorRotation(Direction.Rotation());

			FGameplayTag CooldownTag = FGameplayTag::RequestGameplayTag("Cooldown.AutoAttack");
			Monster->OnCooldown(CooldownTag, AS->GetAttackSpeed());
		}
		else
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag::RequestGameplayTag("Event.Montage.AttackHit"));
		WaitEventTask->EventReceived.AddDynamic(this, &UGA_MonsterState_Attack::OnAttackHitEventReceived);
		WaitEventTask->ReadyForActivation();
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_MonsterState_Attack::OnAttackHitEventReceived(FGameplayEventData Payload)
{
	ABaseMonster* Monster = Cast<ABaseMonster>(GetOwningActorFromActorInfo());
	if (IsValid(Monster) && DamageEffectClass)
	{
		AActor* Target = Monster->GetTargetPlayer();
		if (IsValid(Target))
		{
			UAbilitySystemComponent* ASC = Monster->GetAbilitySystemComponent();
			FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
			ContextHandle.AddInstigator(Monster, Monster);
			FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(DamageEffectClass, 1, ContextHandle);

			if (SpecHandle.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target));
			}
		}
	}
}