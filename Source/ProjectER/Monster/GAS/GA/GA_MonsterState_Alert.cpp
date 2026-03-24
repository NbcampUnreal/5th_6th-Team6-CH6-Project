#include "Monster/GAS/GA/GA_MonsterState_Alert.h"

UGA_MonsterState_Alert::UGA_MonsterState_Alert()
{
	StateInitData.MonsterAssetTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability.Action.Alert"));
	StateInitData.MontageType = EMonsterMontageType::Alert;
	StateInitData.NiagaraCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Particle.Action.Alert");
	StateInitData.SoundCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Sound.Action.Alert");
	StateInitData.WaitTag = FGameplayTag::RequestGameplayTag("State.Action.Alert");
	bIsWaitTag = true;
	SetAssetTags(StateInitData.MonsterAssetTags);
}
