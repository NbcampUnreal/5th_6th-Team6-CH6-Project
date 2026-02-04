// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillBase.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "SkillSystem/SkillDataAsset.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"


#include "AbilitySystemLog.h" // GAS 관련 로그 확인용

USkillBase::USkillBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	CastingTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Casting"));
	ActiveTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Active"));
	StatusTag = FGameplayTag::RequestGameplayTag(FName("Skill.Data.IncomingStatus"));
}

void USkillBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ENetMode NetMode = (GetWorld() != nullptr) ? GetWorld()->GetNetMode() : NM_Standalone;

	FString NetModeStr;
	switch (NetMode)
	{
	case NM_Client: NetModeStr = TEXT("Client"); break;
	case NM_DedicatedServer: NetModeStr = TEXT("DedicatedServer"); break;
	case NM_ListenServer: NetModeStr = TEXT("ListenServer"); break;
	case NM_Standalone: NetModeStr = TEXT("Standalone"); break;
	default: NetModeStr = TEXT("Unknown"); break;
	}

	FString SideStr = HasAuthority(&ActivationInfo) ? TEXT("SERVER") : TEXT("CLIENT");
	FString ActorName = (ActorInfo && ActorInfo->AvatarActor.IsValid()) ? ActorInfo->AvatarActor->GetName() : TEXT("Unknown");

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
}

void USkillBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bWasCancelled == true)
	{
		OnCancelAbility();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USkillBase::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (Spec.SourceObject.IsValid())
	{
		USkillDataAsset* SkillDataAsset = Cast<USkillDataAsset>(Spec.SourceObject);
		if (IsValid(SkillDataAsset))
		{
			CachedConfig = SkillDataAsset->SkillConfig;
		}
	}
}


//void USkillBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
//{
//	Super::PostEditChangeProperty(PropertyChangedEvent);
//}

void USkillBase::ExecuteSkill()
{
	if (IsValid(CachedConfig) == false) return;
	if (CachedConfig->GetExcutionEffects().Num() <= 0) return;
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (IsValid(Avatar) == false) return;

	ApplyEffectsToActor(Avatar, CachedConfig->GetExcutionEffects());
}

void USkillBase::OnActiveTagAdded()
{
	UE_LOG(LogTemp, Warning, TEXT("USkillBase::OnActiveTagAdded"));
	if (CachedConfig->Data.bIsUseCasting)
	{
		if (GetAbilitySystemComponentFromActorInfo()->HasMatchingGameplayTag(CastingTag))
		{
			RemoveTagFromOwner(CastingTag);
			ExecuteSkill();
		}
		else {
			UE_LOG(LogTemp, Warning, TEXT("USkillBase::GetAbilitySystemComponentFromActorInfo()->HasMatchingGameplayTag(CastingTag) is false"));
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		}
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("USkillBase::CachedConfig->Data.bIsUseCasting is false"));
	}
}

void USkillBase::PlayAnimMontage()
{
	UE_LOG(LogTemp, Warning, TEXT("USkillBase::PlayAnimMontage"));
	UAbilityTask_PlayMontageAndWait* PlayTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("SkillAnimation"), CachedConfig->Data.AnimMontage);
	PlayTask->ReadyForActivation();
}

void USkillBase::SetWaitActiveTagTask()
{
	UE_LOG(LogTemp, Warning, TEXT("USkillBase::SetWaitActiveTagTask"));
	UAbilityTask_WaitGameplayTagAdded* WaitTagAdd = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(this, ActiveTag);
	WaitTagAdd->Added.AddDynamic(this, &USkillBase::OnActiveTagAdded);
	WaitTagAdd->ReadyForActivation();
}

void USkillBase::PrepareToActiveSkill()
{
	//SetWaitActiveTagTask();
	//PlayAnimMontage();
	////캐스팅이 없는 스킬이면 Active 태그를 바로 붙여서 즉시 발동
	//if (CachedConfig->Data.bIsUseCasting == false)
	//{
	//	ExecuteSkill();
	//	AddTagToOwner(ActiveTag);
	//}

	PlayAnimMontage();

	if (CachedConfig->Data.bIsUseCasting)
	{
		SetWaitActiveTagTask();
	}
	else
	{
		ExecuteSkill();
		AddTagToOwner(ActiveTag);
	}
}

void USkillBase::ApplyEffectsToActors(TSet<TObjectPtr<AActor>> Actors, const TArray<TObjectPtr<USkillEffectDataAsset>>& SkillEffectDataAssets)
{
	UE_LOG(LogTemp, Warning, TEXT("ApplyEffectsToActors"));
	if (Actors.Num() <= 0 || SkillEffectDataAssets.Num() <= 0) return;

	FGameplayAbilityTargetDataHandle TargetDataHandle;
	FGameplayAbilityTargetData_ActorArray* NewData = new FGameplayAbilityTargetData_ActorArray();

	for (AActor* Target : Actors)
	{
		if (IsValid(Target))
		{
			NewData->TargetActorArray.Add(Target);
		}
	}

	TargetDataHandle.Add(NewData);

	UAbilitySystemComponent* InstigatorASC = GetAbilitySystemComponentFromActorInfo();
	for (USkillEffectDataAsset* EffectData : SkillEffectDataAssets)
	{
		if (!EffectData) continue;
		UE_LOG(LogTemp, Warning, TEXT("!EffectData"));
		TArray<FGameplayEffectSpecHandle> SpecHandles = EffectData->MakeSpecs(InstigatorASC, this, GetAvatarActorFromActorInfo());
		for (FGameplayEffectSpecHandle& SpecHandle : SpecHandles)
		{
			if (SpecHandle.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("SpecHandle.IsValid()"));
				ApplyGameplayEffectSpecToTarget(
					CurrentSpecHandle,
					CurrentActorInfo,
					CurrentActivationInfo,
					SpecHandle,
					TargetDataHandle
				);
			}
		}
	}
}

void USkillBase::ApplyEffectsToActor(AActor* Actors, const TArray<TObjectPtr<USkillEffectDataAsset>>& SkillEffectDataAssets)
{
	if (IsValid(Actors) == false) return;

	TSet<TObjectPtr<AActor>> TargetSet;
	TargetSet.Add(Actors);

	ApplyEffectsToActors(TargetSet, SkillEffectDataAssets);
}

FGameplayTag USkillBase::GetInputTag()
{
	return CachedConfig->Data.InputKeyTag;
}

void USkillBase::FinishSkill()
{
	RemoveTagFromOwner(ActiveTag);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USkillBase::OnCancelAbility()
{

}

void USkillBase::AddTagToOwner(FGameplayTag Tag)
{
	if (Tag.IsValid())
	{
		//같은 태그가 여러개 있어도 해당 태그를 1개로 설정
		//GetAbilitySystemComponentFromActorInfo()->AddLooseGameplayTag(Tag, 1);
		GetAbilitySystemComponentFromActorInfo()->SetTagMapCount(Tag, 1);
	}
}

void USkillBase::RemoveTagFromOwner(FGameplayTag Tag)
{
	if (Tag.IsValid())
	{
		//같은 태그가 여러개 있어도 해당 태그를 0개로 설정
		GetAbilitySystemComponentFromActorInfo()->SetTagMapCount(Tag, 0);
		//GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTag(Tag, 1);
	}
}