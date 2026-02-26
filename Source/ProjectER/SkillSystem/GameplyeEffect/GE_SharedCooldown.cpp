// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplyeEffect/GE_SharedCooldown.h"
#include "GameplayEffect.h"

UGE_SharedCooldown::UGE_SharedCooldown()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;

    FSetByCallerFloat SetByCallerValue;
    SetByCallerValue.DataTag = FGameplayTag::RequestGameplayTag(FName("Skill.Data.CoolTime"));

    FGameplayEffectModifierMagnitude Magnitude(SetByCallerValue);
    DurationMagnitude = Magnitude;
}
