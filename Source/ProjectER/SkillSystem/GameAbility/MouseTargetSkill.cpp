// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameAbility/MouseTargetSkill.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"

UMouseTargetSkill::UMouseTargetSkill()
{

}

void UMouseTargetSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	UE_LOG(LogTemp, Log, TEXT("ActivateAbility before Targeted"));
	Targeted();
}

void UMouseTargetSkill::ExecuteSkill()
{
    UMouseTargetSkillConfig* Config = Cast<UMouseTargetSkillConfig>(ChacedConfig);
    if (!Config || AffectedActors.Num() == 0) return;

    UAbilitySystemComponent* InstigatorASC = GetAbilitySystemComponentFromActorInfo();
    const TArray<TObjectPtr<USkillEffectDataAsset>>& EffectDataAssets = Config->GetEffectDataAssets();

    FGameplayAbilityTargetDataHandle TargetDataHandle;
    FGameplayAbilityTargetData_ActorArray* NewData = new FGameplayAbilityTargetData_ActorArray();

    for (AActor* Target : AffectedActors)
    {
        if (IsValid(Target))
        {
            NewData->TargetActorArray.Add(Target);
        }
    }

    TargetDataHandle.Add(NewData);
    for (USkillEffectDataAsset* EffectData : EffectDataAssets)
    {
        if (!EffectData) continue;
        TArray<FGameplayEffectSpecHandle> SpecHandles = EffectData->MakeSpecs(InstigatorASC, this, GetAvatarActorFromActorInfo());

        for (FGameplayEffectSpecHandle& SpecHandle : SpecHandles)
        {
            if (SpecHandle.IsValid())
            {
                ApplyGameplayEffectSpecToTarget(
                    CurrentSpecHandle,
                    CurrentActorInfo,
                    CurrentActivationInfo,
                    SpecHandle,
                    TargetDataHandle
                );
            }
        }
    }
	FinishSkill();
}

void UMouseTargetSkill::Targeted()
{
	UE_LOG(LogTemp, Log, TEXT("Targeted :: Waiting for Event"));

	UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetInputTag());

	if (WaitEventTask)
	{
		WaitEventTask->EventReceived.AddDynamic(this, &UMouseTargetSkill::OnTargetConfirmed);
		WaitEventTask->ReadyForActivation();
	}

}

void UMouseTargetSkill::OnTargetConfirmed(FGameplayEventData Payload)
{
	UE_LOG(LogTemp, Log, TEXT("Event Received! Target Confirming..."));
	APlayerController* PC = GetActorInfo().PlayerController.Get();
	if (!PC) return;

	FHitResult HitResult;
	if (PC->GetHitResultUnderCursor(ECC_Pawn, false, HitResult)) {
		AActor* HitActor = HitResult.GetActor();
		if (HitActor)
		{
			float Distance = FVector::Dist(GetAvatarActorFromActorInfo()->GetActorLocation(), HitActor->GetActorLocation());

			UMouseTargetSkillConfig* Config = Cast<UMouseTargetSkillConfig>(ChacedConfig);
			if (Config)
			{
				float Range = Config->GetRange();
				if (Distance <= Range)
				{
					UE_LOG(LogTemp, Warning, TEXT("적 타겟팅 성공: %s"), *HitActor->GetName());
					AffectedActors.Add(HitActor);
					PrepareToActiveSkill();
				}
			}
		}
	}
	else {
		OnTargetConfirmed(Payload);
	}
}
