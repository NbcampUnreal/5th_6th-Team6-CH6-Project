// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GCN_SpawnNiagaraBySpawnConfig.h"
#include "SkillSystem/GameplayEffectComponent/BaseGECConfig.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeBaseGEC.h"
#include "SkillSystem/SkillNiagaraSpawnHelper.h"

#include "Engine/Blueprint.h"
#include "AbilitySystemStats.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemLog.h"
#include "GameplayCueManager.h"

//#include UE_INLINE_GENERATED_CPP_BY_NAME(GameplayCueNotify_Static)

namespace
{
	enum class ENiagaraCueSettingType : uint8
	{
		None,
		Summoner,
		Range,
		Hit
	};

	ENiagaraCueSettingType ResolveSettingsFromConfig(const UBaseGECConfig* Config, const FGameplayTag& CueTag, FSkillNiagaraSpawnSettings& OutSettings)
	{
		if (!IsValid(Config) || !CueTag.IsValid())
		{
			return ENiagaraCueSettingType::None;
		}

		const USummonRangeBaseConfig* const SummonConfig = Cast<USummonRangeBaseConfig>(Config);
		if (!IsValid(SummonConfig))
		{
			return ENiagaraCueSettingType::None;
		}

		if (SummonConfig->SummonerSpawnVfx.CueTag == CueTag)
		{
			OutSettings = SummonConfig->SummonerSpawnVfx;
			return ENiagaraCueSettingType::Summoner;
		}

		if (SummonConfig->RangeSpawnVfx.CueTag == CueTag)
		{
			OutSettings = SummonConfig->RangeSpawnVfx;
			return ENiagaraCueSettingType::Range;
		}

		if (SummonConfig->HitTargetVfx.CueTag == CueTag)
		{
			OutSettings = SummonConfig->HitTargetVfx;
			return ENiagaraCueSettingType::Hit;
		}

		return ENiagaraCueSettingType::None;
	}
}

bool UGCN_SpawnNiagaraBySpawnConfig::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (!IsValid(MyTarget))
	{
		//UE_LOG(LogTemp, Error, TEXT("GCN_SpawnNiagara: MyTarget is NULL!"));
		return false;
	}

	ENetMode NetMode = (MyTarget) ? MyTarget->GetNetMode() : NM_Standalone;
	if (MyTarget->HasAuthority() && NetMode == NM_DedicatedServer)
	{
		// 데디서버에서는 시각 효과를 스폰하지 않으므로 여기서 리턴되는 것이 정상입니다.
		//UE_LOG(LogTemp, Log, TEXT("GCN_SpawnNiagara: Skipping Spawn on Dedicated Server."));
		return false;
	}

	UWorld* const World = MyTarget->GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	const UBaseGECConfig* const Config = Cast<UBaseGECConfig>(Parameters.SourceObject);
	if (!IsValid(Config))
	{
		//UE_LOG(LogTemp, Warning, TEXT("GCN_SpawnNiagara: SourceObject is Invalid!"));
		return false;
	}

	FSkillNiagaraSpawnSettings SpawnSettings;
	const ENiagaraCueSettingType SettingType = ResolveSettingsFromConfig(Config, GameplayCueTag, SpawnSettings);
	if (SettingType == ENiagaraCueSettingType::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("GCN_SpawnNiagara: SettingType is None! Tag Mismatch? (GCN Tag: %s)"), *GameplayCueTag.ToString());
		return false;
	}

	const AActor* const EffectCauser = Cast<AActor>(Parameters.EffectCauser.Get());
	const AActor* const SourceActor = IsValid(EffectCauser) ? EffectCauser : MyTarget;

	FTransform SourceTransform = IsValid(SourceActor) ? SourceActor->GetActorTransform() : FTransform::Identity;
	SourceTransform.SetLocation(Parameters.Location);

	const FVector* OptionalLookAtTarget = nullptr;
	FVector LookAtTargetLocation = FVector::ZeroVector;

	if (SettingType == ENiagaraCueSettingType::Hit && IsValid(SourceActor))
	{
		LookAtTargetLocation = SourceActor->GetActorLocation();
		OptionalLookAtTarget = &LookAtTargetLocation;
	}

	SkillNiagaraSpawnHelper::SpawnNiagaraBySettings(World, SpawnSettings, SourceTransform, SourceActor, OptionalLookAtTarget);
	return true;
}
