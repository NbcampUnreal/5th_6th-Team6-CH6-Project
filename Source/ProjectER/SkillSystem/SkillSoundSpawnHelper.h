// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
 
class UWorld;
class AActor;
class USceneComponent;

struct FSkillSoundSpawnSettings;

namespace SkillSoundSpawnHelper
{
	/**
	 * Settings에 정의된 설정대로 사운드를 재생합니다.
	 * @param World			사운드가 재생될 월드
	 * @param Settings		사운드 설정 (구조체)
	 * @param SourceTransform 기준이 되는 트랜스폼 (주로 시전자의 트랜스폼)
	 * @param SourceActor	어태치 대상이 될 액터
	 * @param OptionalLookAtTarget	LookAt 모드일 때 바라볼 타겟 위치
	 * @param AttachTarget	직접 지정하는 어태치 컴포넌트
	 */
	void PlaySoundBySettings(UWorld* World, const FSkillSoundSpawnSettings& Settings, const FTransform& SourceTransform, const AActor* SourceActor = nullptr, const FVector* OptionalLookAtTarget = nullptr, USceneComponent* AttachTarget = nullptr);
}
