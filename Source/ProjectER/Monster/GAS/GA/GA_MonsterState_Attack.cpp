#include "Monster/GAS/GA/GA_MonsterState_Attack.h"

UGA_MonsterState_Attack::UGA_MonsterState_Attack()
{
	StateInitData.MonsterAssetTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability.Action.AutoAttack"));
	StateInitData.MontageType = EMonsterMontageType::Attack;
	StateInitData.NiagaraCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Particle.Skill.AutoAttack");
	StateInitData.SoundCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Sound.Skill.AutoAttack");
	StateInitData.WaitTag = FGameplayTag::RequestGameplayTag("State.Action.Alert");

	SetAssetTags(StateInitData.MonsterAssetTags);
}
