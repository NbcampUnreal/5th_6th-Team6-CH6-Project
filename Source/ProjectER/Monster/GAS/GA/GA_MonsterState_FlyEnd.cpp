#include "Monster/GAS/GA/GA_MonsterState_FlyEnd.h"

UGA_MonsterState_FlyEnd::UGA_MonsterState_FlyEnd()
{
	StateInitData.MonsterAssetTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability.Action.FlyEnd"));
	StateInitData.MontageType = EMonsterMontageType::FlyEnd;
	StateInitData.NiagaraCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Particle.Action.FlyEnd");
	StateInitData.SoundCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Sound.Action.FlyEnd");
	StateInitData.WaitTag = FGameplayTag::RequestGameplayTag("State.Action.FlyEnd");

	SetAssetTags(StateInitData.MonsterAssetTags);
}
