// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GCN_PlaySoundBySpawnConfig.generated.h"
 
class AActor;
struct FGameplayCueParameters;

/**
 * USkillSoundSpawnConfig를 사용하여 사운드를 재생하는 GameplayCueNotify_Static 클래스입니다.
 */
UCLASS()
class PROJECTER_API UGCN_PlaySoundBySpawnConfig : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};
