// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "SkillSystem/GameplayEffectComponent/BaseGECConfig.h"

UBaseGEC::UBaseGEC()
{
	ConfigClass = UTESTConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> UBaseGEC::GetRequiredConfigClass() const
{
	return UTESTConfig::StaticClass();
}
