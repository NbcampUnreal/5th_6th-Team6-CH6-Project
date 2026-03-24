#include "Monster/GAS/GA/GA_MonsterState_Idle.h"

UGA_MonsterState_Idle::UGA_MonsterState_Idle()
{
	StateInitData.MonsterAssetTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability.Action.Idle"));
	StateInitData.MontageType = EMonsterMontageType::Idle;
	StateInitData.NiagaraCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Particle.Action.Idle");
	StateInitData.SoundCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Sound.Action.Idle");
	StateInitData.WaitTag = FGameplayTag::RequestGameplayTag("State.Action.Idle");

	SetAssetTags(StateInitData.MonsterAssetTags);
}
