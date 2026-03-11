// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GCN_SpawnNiagaraBySpawnConfig.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTER_API UGCN_SpawnNiagaraBySpawnConfig : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
public:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};
