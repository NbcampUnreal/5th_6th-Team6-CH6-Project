#include "Monster/GAS/GA/GA_MonsterState_Attack.h"
#include "Monster/BaseMonster.h"
#include "Monster/GAS/AttributeSet/BaseMonsterAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGA_MonsterState_Attack::UGA_MonsterState_Attack()
{
	StateInitData.MonsterAssetTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability.Action.AutoAttack"));
	StateInitData.MontageType = EMonsterMontageType::Attack;
	StateInitData.NiagaraCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Particle.Skill.AutoAttack");
	StateInitData.SoundCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Sound.Skill.AutoAttack");
	StateInitData.WaitTag = FGameplayTag::RequestGameplayTag("State.Action.Attack");
	bIsUseWaitTag = false;
	SetAssetTags(StateInitData.MonsterAssetTags);
}

void UGA_MonsterState_Attack::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);
}

void UGA_MonsterState_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ABaseMonster* Monster = Cast<ABaseMonster>(GetOwningActorFromActorInfo());
	if (IsValid(Monster) == false || IsValid(Monster->MonsterData) == false)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag::RequestGameplayTag("Event.Montage.AttackHit"));
	WaitEventTask->EventReceived.AddDynamic(this, &UGA_MonsterState_Attack::OnAttackHitEventReceived);
	WaitEventTask->ReadyForActivation();

	AActor* Target = Monster->GetTargetPlayer();
	if (IsValid(Target))
	{
		UBaseAttributeSet* AS = Monster->GetAttributeSet();
		if (IsValid(AS))
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

		if (!Monster->GetIsFirstAttack())
		{
			Monster->SetIsFirstAttack(true);
		}
		Monster->SetAttackCount(Monster->GetAttackCount() + 1);
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

}

void UGA_MonsterState_Attack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
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

void UGA_MonsterState_Attack::OnMontageCompleted()
{
	Super::EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_MonsterState_Attack::OnMontageBlendOut()
{
	Super::EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_MonsterState_Attack::OnMontageInterrupt()
{
	Super::EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_MonsterState_Attack::OnMontageCancel()
{
	Super::EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
