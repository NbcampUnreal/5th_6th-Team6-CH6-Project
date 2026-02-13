// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "Skillsystem/GameAbility/SkillBase.h"
#include "SkillSystem/GameAbility/MouseTargetSkill.h"
#include "SkillSystem/GameAbility/MouseClickSkill.h"

UBaseSkillConfig::UBaseSkillConfig()
{
	AbilityClass = USkillBase::StaticClass();
}

UMouseTargetSkillConfig::UMouseTargetSkillConfig()
{
	AbilityClass = UMouseTargetSkill::StaticClass();
	Data.SkillActivationType = ESkillActivationType::Targeted;
}

UMouseClickSkillConfig::UMouseClickSkillConfig()
{
	AbilityClass = UMouseClickSkill::StaticClass();
	Data.SkillActivationType = ESkillActivationType::PointClick;
}
