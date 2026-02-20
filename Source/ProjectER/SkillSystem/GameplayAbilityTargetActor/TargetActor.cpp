// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayAbilityTargetActor/TargetActor.h"
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
}

bool ATargetActor::TryConfirmMouseTarget()
{
    UMouseTargetSkill* MouseSkill = Cast<UMouseTargetSkill>(OwningAbility);
    checkf(IsValid(MouseSkill), TEXT("ATargetActor::ConfirmTargetingAndContinue - MouseSkill Is Not Valid"));

    AActor* ValidTarget = MouseSkill->GetTargetUnderCursorInRange();

    if (ValidTarget)
    {
        FGameplayAbilityTargetDataHandle Handle = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(ValidTarget);
        TargetDataReadyDelegate.Broadcast(Handle);
        return true;
    }

    return false;
}
