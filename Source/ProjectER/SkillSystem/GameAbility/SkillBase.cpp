// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillBase.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
//#include "SkillSystem/SkillDataAsset.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"

USkillBase::USkillBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	CastingTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Casting"));
	ActiveTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Active"));
	StatusTag = FGameplayTag::RequestGameplayTag(FName("Skill.Data.IncomingStatus"));
}

void USkillBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	//switch (SkillData.SkillActivationType)
	//{
	//case ESkillActivationType::Instant:
	//	Instant();
	//	break;

	//case ESkillActivationType::Targeted:
	//	Targeted();
	//	break;

	//case ESkillActivationType::PointClick:
	//	PointClick();
	//	break;

	//case ESkillActivationType::ClickAndDrag:
	//	ClickAndDrag();
	//	break;

	//case ESkillActivationType::Holding:
	//	Holding();
	//	break;
	//}
}

void USkillBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bWasCancelled == true)
	{
		OnCancelAbility();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USkillBase::Assign(USkillDataAsset* DataAsset)
{
	//SkillData = DataAsset->SkillData;
}

//void USkillBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
//{
//	Super::PostEditChangeProperty(PropertyChangedEvent);
//}

void USkillBase::ExecuteSkill()
{
	
}

void USkillBase::OnActiveTagAdded()
{
	if (SkillConfig.Get()->SkillData.bIsUseCasting)
	{
		if (GetAbilitySystemComponentFromActorInfo()->HasMatchingGameplayTag(CastingTag))
		{
			RemoveTagFromOwner(CastingTag);
			ExecuteSkill();
		}
		else {
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		}
	}
}

void USkillBase::PlayAnimMontage()
{
	UAbilityTask_PlayMontageAndWait* PlayTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("SkillAnimation"), SkillConfig.Get()->SkillData.AnimMontage);
	PlayTask->ReadyForActivation();
}

void USkillBase::SetWaitActiveTagTask()
{
	UAbilityTask_WaitGameplayTagAdded* WaitTagAdd = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(this, ActiveTag);
	WaitTagAdd->Added.AddDynamic(this, &USkillBase::OnActiveTagAdded);
	WaitTagAdd->ReadyForActivation();
}

void USkillBase::PrepareToActiveSkill()
{
	SetWaitActiveTagTask();

	//캐스팅이 없는 스킬이면 Active 태그를 바로 붙여서 즉시 발동
	if (SkillConfig.Get()->SkillData.bIsUseCasting == false)
	{
		AddTagToOwner(ActiveTag);
	}
}

void USkillBase::Instant()
{
	PrepareToActiveSkill();
}

//void USkillBase::Targeted()
//{
//	// 1. 입력을 기다립니다. (사용자가 왼쪽 마우스 버튼을 누를 때까지)
//	// bTestInputAlreadyPressed를 false로 해서, 함수 호출 이전에 누르고 있던 건 무시합니다.
//	UAbilityTask_WaitInputPress* WaitInputTask = UAbilityTask_WaitInputPress::WaitInputPress(this, false);
//
//	if (WaitInputTask)
//	{
//		// 2. 클릭했을 때 실행될 함수 연결
//		WaitInputTask->OnPress.AddDynamic(this, &USkillBase::OnTargetConfirmed);
//		WaitInputTask->ReadyForActivation();
//		UE_LOG(LogTemp, Log, TEXT("타겟팅 모드 진입: 적을 클릭하세요."));
//	}
//}
//
//void USkillBase::OnTargetConfirmed(float ElapsedTime)
//{
//	//ElapsedTime는 빌리티가 실행된 순간부터 ~ 실제 키가 눌린 순간까지의 시간 차이인데 안씁니다.
//	APlayerController* PC = GetActorInfo().PlayerController.Get();
//	if (!PC) return;
//
//	//float Distance = FVector::Dist(GetAvatarActorFromActorInfo()->GetActorLocation(), HitActor->GetActorLocation());
//	//if (Distance <= CachedConfig->MaxRange)
//	//{
//	//	// 발사!
//	//}
//	//else
//	//{
//	//	// "사거리가 너무 멉니다" 알림
//	//}
//
//	FHitResult HitResult;
//	if (PC->GetHitResultUnderCursor(ECC_Pawn, false, HitResult)) {
//		AActor* HitActor = HitResult.GetActor();
//		if (HitActor)
//		{
//			UE_LOG(LogTemp, Warning, TEXT("적 타겟팅 성공: %s"), *HitActor->GetName());
//			PrepareToActiveSkill();
//			TargetDatas.Add(HitActor);
//			ExecuteSkill();
//		}
//	}
//}

//void USkillBase::PointClick()
//{
//}
//
//void USkillBase::ClickAndDrag()
//{
//}
//
//void USkillBase::Holding()
//{
//}

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
		GetAbilitySystemComponentFromActorInfo()->AddLooseGameplayTag(Tag, 1);
	}
}

void USkillBase::RemoveTagFromOwner(FGameplayTag Tag)
{
	if (Tag.IsValid())
	{
		//같은 태그가 여러개 있어도 해당 태그를 0개로 설정
		GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTag(Tag, 0);
	}
}