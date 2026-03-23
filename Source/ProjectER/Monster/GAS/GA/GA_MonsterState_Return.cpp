#include "Monster/GAS/GA/GA_MonsterState_Return.h"

UGA_MonsterState_Return::UGA_MonsterState_Return()
{
	StateInitData.MonsterAssetTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability.Action.Return"));
	StateInitData.MontageType = EMonsterMontageType::Move;
	StateInitData.NiagaraCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Particle.Action.Move");
	StateInitData.SoundCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Sound.Action.Move");
	StateInitData.WaitTag = FGameplayTag::RequestGameplayTag("State.Action.Move");

	SetAssetTags(StateInitData.MonsterAssetTags);
}
