#include "Monster/GAS/GA/GA_MonsterState_FlySkill.h"

UGA_MonsterState_FlySkill::UGA_MonsterState_FlySkill()
{
	StateInitData.MonsterAssetTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability.Action.FlyAttack"));
	StateInitData.MontageType = EMonsterMontageType::FlyAttack;
	StateInitData.NiagaraCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Particle.Action.FlyAttack");
	StateInitData.SoundCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Sound.Action.FlyAttack");
	StateInitData.WaitTag = FGameplayTag::RequestGameplayTag("State.Action.FlyAttack");

	SetAssetTags(StateInitData.MonsterAssetTags);
}
