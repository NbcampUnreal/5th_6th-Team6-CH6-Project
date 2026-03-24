#include "Monster/GAS/GA/GA_MonsterState_FlyStart.h"

UGA_MonsterState_FlyStart::UGA_MonsterState_FlyStart()
{
	StateInitData.MonsterAssetTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability.Action.FlyStart"));
	StateInitData.MontageType = EMonsterMontageType::FlyStart;
	StateInitData.NiagaraCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Particle.Action.FlyStart");
	StateInitData.SoundCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Sound.Action.FlyStart");
	StateInitData.WaitTag = FGameplayTag::RequestGameplayTag("State.Action.FlyStart");

	SetAssetTags(StateInitData.MonsterAssetTags);
}
