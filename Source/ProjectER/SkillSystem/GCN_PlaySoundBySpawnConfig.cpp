#include "SkillSystem/GCN_PlaySoundBySpawnConfig.h"
#include "SkillSystem/SkillSoundSpawnConfig.h"
#include "SkillSystem/SkillSoundSpawnHelper.h"
#include "AbilitySystemComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"

namespace
{
	const USkillSoundSpawnConfig* GetSoundSpawnConfigFromParameters(const FGameplayCueParameters& Parameters)
	{
		return Cast<USkillSoundSpawnConfig>(Parameters.SourceObject.Get());
	}

	bool ShouldSkipSoundOnServer(const AActor* MyTarget)
	{
		if (!IsValid(MyTarget))
		{
			return true;
		}
		const ENetMode NetMode = MyTarget->GetNetMode();
		return MyTarget->HasAuthority() && NetMode == NM_DedicatedServer;
	}
}

bool UGCN_PlaySoundBySpawnConfig::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (ShouldSkipSoundOnServer(MyTarget))
	{
		return false;
	}

	UWorld* const World = MyTarget->GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	const USkillSoundSpawnConfig* const SpawnConfig = GetSoundSpawnConfigFromParameters(Parameters);
	if (!IsValid(SpawnConfig))
	{
		return false;
	}

	const FSkillSoundSpawnSettings SpawnSettings = SpawnConfig->ToSettings();
	if (SpawnSettings.Sound.IsNull())
	{
		return false;
	}

	const AActor* const EffectCauser = Cast<AActor>(Parameters.EffectCauser.Get());
	const AActor* const Instigator = Cast<AActor>(Parameters.Instigator.Get());

	static const FGameplayTag TagSummoner = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Skill.Summoner"));
	static const FGameplayTag TagHitTarget = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Skill.HitTarget"));
	static const FGameplayTag TagRange = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Skill.Range"));
	
	const AActor* SourceActor = nullptr;
	if (Parameters.OriginalTag.MatchesTag(TagSummoner))
	{
	    SourceActor = IsValid(Instigator) ? Instigator : MyTarget;
	}
	else if (Parameters.OriginalTag.MatchesTag(TagHitTarget))
	{
	    SourceActor = MyTarget;
	}
	else // 기본값 (Range 포함)
	{
	    SourceActor = EffectCauser;
	}

	// 3. Transform 설정 및 Location 예외 처리
	FTransform SourceTransform = IsValid(SourceActor) ? SourceActor->GetActorTransform() : FTransform(FRotator::ZeroRotator, Parameters.Location);
	
	// [핵심] Range일 때만 전달받은 위치(마우스 클릭 지점 등)로 강제 고정
	if (Parameters.OriginalTag.MatchesTag(TagRange))
	{
	    SourceTransform.SetLocation(Parameters.Location);
	}

	SkillSoundSpawnHelper::PlaySoundBySettings(World, SpawnSettings, SourceTransform, SourceActor, nullptr, Parameters.TargetAttachComponent.Get());
	return true;
}

bool UGCN_PlaySoundBySpawnConfig::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	return OnExecute_Implementation(MyTarget, Parameters);
}

bool UGCN_PlaySoundBySpawnConfig::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (!IsValid(MyTarget))
	{
		return false;
	}

	const USkillSoundSpawnConfig* const SpawnConfig = GetSoundSpawnConfigFromParameters(Parameters);
	if (!IsValid(SpawnConfig) || SpawnConfig->Sound.IsNull())
	{
		return false;
	}

	USoundBase* const LoadedSound = SpawnConfig->Sound.LoadSynchronous();
	if (!IsValid(LoadedSound))
	{
		return false;
	}

	// 캐릭터에서 동일한 Sound를 가진 오디오 컴포넌트를 찾아 정지
	TArray<UAudioComponent*> AudioComponents;
	MyTarget->GetComponents<UAudioComponent>(AudioComponents);
	for (UAudioComponent* AC : AudioComponents)
	{
		if (IsValid(AC) && AC->Sound == LoadedSound)
		{
			AC->Stop();
		}
	}

	return true;
}
