// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
struct FSkillNiagaraSpawnSettings;

namespace SkillNiagaraSpawnHelper
{
	void SpawnNiagaraBySettings(UWorld* World, const FSkillNiagaraSpawnSettings& Settings, const FTransform& SourceTransform, const AActor* SourceActor = nullptr, const FVector* OptionalLookAtTarget = nullptr, USceneComponent* AttachTarget = nullptr);
}
