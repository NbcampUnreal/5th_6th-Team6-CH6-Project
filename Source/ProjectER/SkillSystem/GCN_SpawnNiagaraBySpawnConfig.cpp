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
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"

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
	const AActor* const Instigator = Cast<AActor>(Parameters.Instigator.Get());

	// CueTag를 기반으로 SourceActor(부착/기준 대상) 결정:
	// - "Summoner" → 시전자(Instigator) 기준 부착
	// - "HitTarget" → 피격 대상(MyTarget) 기준 부착
	// - 그 외(Range 등) → EffectCauser(범위 액터 등) 기준 부착
	const FString TagStr = Parameters.OriginalTag.ToString();
	const AActor* SourceActor = nullptr;
	if (TagStr.Contains(TEXT("Summoner")))
	{
		SourceActor = IsValid(Instigator) ? Instigator : MyTarget;
	}
	else if (TagStr.Contains(TEXT("HitTarget")))
	{
		SourceActor = MyTarget;
	}
	else
	{
		SourceActor = IsValid(EffectCauser) ? EffectCauser : MyTarget;
	}

	FTransform SourceTransform = IsValid(SourceActor) ? SourceActor->GetActorTransform() : FTransform::Identity;
	SourceTransform.SetLocation(Parameters.Location);

	SkillNiagaraSpawnHelper::SpawnNiagaraBySettings(World, SpawnSettings, SourceTransform, SourceActor, nullptr, Parameters.TargetAttachComponent.Get());
	return true;
}

bool UGCN_SpawnNiagaraBySpawnConfig::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	// 1. 시각 효과 스폰
	OnExecute_Implementation(MyTarget, Parameters);

	// 2. 클라이언트-사이드 이동 동기화 (호스트가 아닌 경우에만 로컬 RootMotionSource 적용)
	if (IsValid(MyTarget) && !MyTarget->HasAuthority())
	{
		ACharacter* const Character = Cast<ACharacter>(MyTarget);
		UCharacterMovementComponent* const CMC = IsValid(Character) ? Character->GetCharacterMovement() : nullptr;

		// 방향(Normal)과 속도(RawMagnitude)가 유효한 경우에만 실행
		if (IsValid(CMC) && !Parameters.Normal.IsNearlyZero() && Parameters.RawMagnitude > 0.0f)
		{
			TSharedPtr<FRootMotionSource_ConstantForce> ConstantForce = MakeShared<FRootMotionSource_ConstantForce>();
			ConstantForce->InstanceName = FName(TEXT("ConstantForceMoveGEC_Client"));
			ConstantForce->AccumulateMode = ERootMotionAccumulateMode::Override;
			ConstantForce->Priority = 5;
			ConstantForce->Force = Parameters.Normal * Parameters.RawMagnitude;
			ConstantForce->Duration = (Parameters.NormalizedMagnitude > 0.0f) ? Parameters.NormalizedMagnitude : 5.0f;
			ConstantForce->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity;

			CMC->ApplyRootMotionSource(ConstantForce);
		}
	}

	return true;
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

	// 2. 클라이언트-사이드 이동 종료
	if (IsValid(MyTarget) && !MyTarget->HasAuthority())
	{
		ACharacter* const Character = Cast<ACharacter>(MyTarget);
		if (UCharacterMovementComponent* const CMC = IsValid(Character) ? Character->GetCharacterMovement() : nullptr)
		{
			CMC->RemoveRootMotionSource(FName(TEXT("ConstantForceMoveGEC_Client")));
		}
	}

	return true;
}
