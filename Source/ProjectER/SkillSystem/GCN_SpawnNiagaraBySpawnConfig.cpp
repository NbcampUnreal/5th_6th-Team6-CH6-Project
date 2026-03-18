// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GCN_SpawnNiagaraBySpawnConfig.h"
#include "SkillSystem/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/SkillNiagaraSpawnHelper.h"

#include "Engine/Blueprint.h"
#include "AbilitySystemStats.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemLog.h"
#include "GameplayCueManager.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

//#include UE_INLINE_GENERATED_CPP_BY_NAME(GameplayCueNotify_Static)

namespace
{
	/**
	 * SourceObject에서 USkillNiagaraSpawnConfig를 직접 가져옵니다.
	 * 기존 ResolveSettingsFromConfig를 완전히 대체합니다.
	 */
	const USkillNiagaraSpawnConfig* GetSpawnConfigFromParameters(const FGameplayCueParameters& Parameters)
	{
		return Cast<USkillNiagaraSpawnConfig>(Parameters.SourceObject.Get());
	}

	/** 공통 서버 체크 */
	bool ShouldSkipOnServer(const AActor* MyTarget)
	{
		if (!IsValid(MyTarget))
		{
			return true;
		}
		const ENetMode NetMode = MyTarget->GetNetMode();
		return MyTarget->HasAuthority() && NetMode == NM_DedicatedServer;
	}
}

bool UGCN_SpawnNiagaraBySpawnConfig::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (ShouldSkipOnServer(MyTarget))
	{
		return false;
	}

	UWorld* const World = MyTarget->GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	const USkillNiagaraSpawnConfig* const SpawnConfig = GetSpawnConfigFromParameters(Parameters);
	if (!IsValid(SpawnConfig))
	{
		return false;
	}

	const FSkillNiagaraSpawnSettings SpawnSettings = SpawnConfig->ToSettings();
	if (SpawnSettings.NiagaraSystem.IsNull())
	{
		return false;
	}

	const AActor* const EffectCauser = Cast<AActor>(Parameters.EffectCauser.Get());
	const AActor* const SourceActor = IsValid(EffectCauser) ? EffectCauser : MyTarget;

	FTransform SourceTransform = IsValid(SourceActor) ? SourceActor->GetActorTransform() : FTransform::Identity;
	SourceTransform.SetLocation(Parameters.Location);

	SkillNiagaraSpawnHelper::SpawnNiagaraBySettings(World, SpawnSettings, SourceTransform, SourceActor);
	return true;
}

bool UGCN_SpawnNiagaraBySpawnConfig::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	// OnExecute와 동일한 스폰 로직 — MovingVfx 루핑 시작 시 호출
	return OnExecute_Implementation(MyTarget, Parameters);
}

bool UGCN_SpawnNiagaraBySpawnConfig::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (!IsValid(MyTarget))
	{
		return false;
	}

	const USkillNiagaraSpawnConfig* const SpawnConfig = GetSpawnConfigFromParameters(Parameters);
	if (!IsValid(SpawnConfig) || SpawnConfig->NiagaraSystem.IsNull())
	{
		return false;
	}

	UNiagaraSystem* const LoadedSystem = SpawnConfig->NiagaraSystem.LoadSynchronous();
	if (!IsValid(LoadedSystem))
	{
		return false;
	}

	// 캐릭터에서 동일한 NiagaraSystem을 가진 컴포넌트를 찾아 Deactivate
	TArray<UNiagaraComponent*> NCs;
	MyTarget->GetComponents<UNiagaraComponent>(NCs);
	for (UNiagaraComponent* NC : NCs)
	{
		if (IsValid(NC) && NC->GetAsset() == LoadedSystem)
		{
			NC->Deactivate();
		}
	}

	return true;
}
