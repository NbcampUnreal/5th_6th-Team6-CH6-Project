// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameAbility/MouseTargetSkill.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "SkillSystem/AGameplayAbilityTargetActor/TargetActor.h"

UMouseTargetSkill::UMouseTargetSkill()
{

}

void UMouseTargetSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (CastInstantly() == false)
	{
		SetWaitTargetTask();
	}
}

void UMouseTargetSkill::ExecuteSkill()
{
	UMouseTargetSkillConfig* Config = Cast<UMouseTargetSkillConfig>(CachedConfig);
	if (!Config || AffectedActors.Num() == 0) return;

	UAbilitySystemComponent* InstigatorASC = GetAbilitySystemComponentFromActorInfo();
	const TArray<TObjectPtr<USkillEffectDataAsset>>& EffectDataAssets = Config->GetEffectDataAssets();

	FGameplayAbilityTargetDataHandle TargetDataHandle;
	FGameplayAbilityTargetData_ActorArray* NewData = new FGameplayAbilityTargetData_ActorArray();

	for (AActor* Target : AffectedActors)
	{
		if (IsValid(Target))
		{
			NewData->TargetActorArray.Add(Target);
		}
	}

	TargetDataHandle.Add(NewData);
	for (USkillEffectDataAsset* EffectData : EffectDataAssets)
	{
		if (!EffectData) continue;
		TArray<FGameplayEffectSpecHandle> SpecHandles = EffectData->MakeSpecs(InstigatorASC, this, GetAvatarActorFromActorInfo());

		for (FGameplayEffectSpecHandle& SpecHandle : SpecHandles)
		{
			if (SpecHandle.IsValid())
			{
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
	FinishSkill();
}

void UMouseTargetSkill::FinishSkill()
{
	Super::FinishSkill();
	AffectedActors.Empty();
}

void UMouseTargetSkill::SetWaitTargetTask()
{
	UAbilityTask_WaitTargetData* WaitTargetTask = UAbilityTask_WaitTargetData::WaitTargetData(
		this,
		TEXT("WaitTargetTask"),
		EGameplayTargetingConfirmation::UserConfirmed,
		ATargetActor::StaticClass()
	);

	WaitTargetTask->ValidData.AddDynamic(this, &UMouseTargetSkill::OnTargetDataReady);
	WaitTargetTask->Cancelled.AddDynamic(this, &UMouseTargetSkill::OnTargetCancelled);

	AGameplayAbilityTargetActor* SpawnedActor = nullptr;
	if (WaitTargetTask->BeginSpawningActor(this, ATargetActor::StaticClass(), SpawnedActor))
	{
		ATargetActor* MyTargetActor = Cast<ATargetActor>(SpawnedActor);
		if (MyTargetActor)
		{
			MyTargetActor->PrimaryPC = Cast<APlayerController>(GetActorInfo().PlayerController);
		}
		WaitTargetTask->FinishSpawningActor(this, SpawnedActor);
	}

	WaitTargetTask->ReadyForActivation();
}

bool UMouseTargetSkill::CastInstantly()
{
	FHitResult HitResult;
	// Tip: Controller가 유효한지 ActorInfo를 통해 한 번 더 확인하는 것이 안전합니다.
	APlayerController* PC = Cast<APlayerController>(GetActorInfo().PlayerController.Get());

	if (PC && PC->GetHitResultUnderCursor(ECC_Pawn, false, HitResult))
	{
		AActor* HitActor = HitResult.GetActor();

		if (IsValid(HitActor))
		{
			UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);

			if (TargetASC)
			{
				AffectedActors.Add(HitActor);
				PrepareToActiveSkill();
				return true;
			}
		}
	}

	return false;
}

void UMouseTargetSkill::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle)
{
	UE_LOG(LogTemp, Log, TEXT("OnTargetDataReady"));
	TArray<AActor*> TargetActors = UAbilitySystemBlueprintLibrary::GetActorsFromTargetData(DataHandle, 0);

	if (TargetActors.Num() <= 0) return;

	for (AActor* Actor : TargetActors)
	{
		AffectedActors.Add(Actor);
	}

	PrepareToActiveSkill();
}

void UMouseTargetSkill::OnTargetCancelled(const FGameplayAbilityTargetDataHandle& DataHandle)
{
	UE_LOG(LogTemp, Log, TEXT("OnTargetCancelled"));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}