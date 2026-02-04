// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/AGameplayAbilityTargetActor/TargetActor.h"
#include "GameFramework/Actor.h"
#include "SkillSystem/GameAbility/MouseTargetSkill.h"
#include "AbilitySystemBlueprintLibrary.h"

ATargetActor::ATargetActor()
{
    //bDestroyOnConfirmation = true;
}

void ATargetActor::BeginPlay()
{
    Super::BeginPlay();
}

void ATargetActor::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);

    if (ShouldProduceTargetData())
    {
        /*UMouseTargetSkill* MouseSkill = Cast<UMouseTargetSkill>(OwningAbility);
        checkf(IsValid(MouseSkill), TEXT("ATargetActor::ConfirmTargetingAndContinue - MouseSkill Is Not Valid"));

        if (AActor* ValidTarget = MouseSkill->GetTargetUnderCursorInRange())
        {
            FGameplayAbilityTargetDataHandle Handle = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(ValidTarget);
            TargetDataReadyDelegate.Broadcast(Handle);
        }
        else
        {
            FGameplayAbilityTargetDataHandle CancelHandle;
            CanceledDelegate.Broadcast(CancelHandle);
        }*/
    }
}

void ATargetActor::ConfirmTargetingAndContinue()
{
    UMouseTargetSkill* MouseSkill = Cast<UMouseTargetSkill>(OwningAbility);
    checkf(IsValid(MouseSkill), TEXT("ATargetActor::ConfirmTargetingAndContinue - MouseSkill Is Not Valid"));

    if (TryConfirmMouseTarget() == false)
    {
        FGameplayAbilityTargetDataHandle CancelHandle;
        CanceledDelegate.Broadcast(CancelHandle);
    }

    //// 사거리 내에 유효한 타겟이 찍혔는가?
    //if (AActor* ValidTarget = MouseSkill->GetTargetUnderCursorInRange())
    //{
    //    FGameplayAbilityTargetDataHandle Handle = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(ValidTarget);
    //    TargetDataReadyDelegate.Broadcast(Handle);
    //}
    //else
    //{
    //    FGameplayAbilityTargetDataHandle CancelHandle;
    //    CanceledDelegate.Broadcast(CancelHandle);
    //}
}

bool ATargetActor::TryConfirmMouseTarget()
{
    UMouseTargetSkill* MouseSkill = Cast<UMouseTargetSkill>(OwningAbility);
    checkf(IsValid(MouseSkill), TEXT("ATargetActor::ConfirmTargetingAndContinue - MouseSkill Is Not Valid"));
    if (AActor* ValidTarget = MouseSkill->GetTargetUnderCursorInRange())
    {
        FGameplayAbilityTargetDataHandle Handle = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(ValidTarget);
        TargetDataReadyDelegate.Broadcast(Handle);
        return true;
    }
    else {
        UE_LOG(LogTemp, Warning, TEXT("TryConfirmMouseTarget is false"));
    }

    return false;
}
