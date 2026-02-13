// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameAbility/MouseClickSkill.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "AbilitySystemComponent.h"

void UMouseClickSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	PrepareDataSetDelegate(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (IsLocallyControlled())
	{
		SendLocationData(GetMouseLocation());
	}
	else {
		//DoNoting
	}
}

void UMouseClickSkill::SendLocationData(FVector TargetLocation)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	FGameplayAbilityTargetDataHandle DataHandle;
	FGameplayAbilityTargetData_LocationInfo* LocData = new FGameplayAbilityTargetData_LocationInfo();
	LocData->TargetLocation.LiteralTransform = FTransform(TargetLocation);
	DataHandle.Add(LocData);

	FScopedPredictionWindow ScopedPrediction(ASC, CurrentActorInfo->IsLocallyControlled());
	ASC->CallServerSetReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey(), DataHandle, FGameplayTag(), ASC->ScopedPredictionKey);
	//ASC->CallReplicatedTargetDataDelegatesIfSet(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
}

bool UMouseClickSkill::IsInRange(const FVector& Location)
{
	return false;
}

void UMouseClickSkill::RotateToLocation(const FVector& Location)
{
}

void UMouseClickSkill::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	check(ASC);
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo)) return;

	FinalLocation = DataHandle.Get(0)->GetEndPoint();

	PrepareToActiveSkill();

	// 마무리 작업
	ASC->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey()).Remove(TargetDataDelegateHandle);
	ASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
}

void UMouseClickSkill::ExecuteSkill()
{
	Super::ExecuteSkill();


}


void UMouseClickSkill::PrepareDataSetDelegate(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	check(ASC);

	TargetDataDelegateHandle = ASC->AbilityTargetDataSetDelegate(Handle, ActivationInfo.GetActivationPredictionKey()).AddUObject(this, &UMouseClickSkill::OnTargetDataReady);
	ASC->CallReplicatedTargetDataDelegatesIfSet(Handle, ActivationInfo.GetActivationPredictionKey());
}

FVector UMouseClickSkill::GetMouseLocation() const
{
	APlayerController* PC = GetActorInfo().PlayerController.Get();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!PC || !Avatar) return FVector::ZeroVector;

	FVector WorldLoc, WorldDir;
	if (PC->DeprojectMousePositionToWorld(WorldLoc, WorldDir))
	{
		FVector PlaneOrigin = Avatar->GetActorLocation(); // 캐릭터 발밑 높이
		FVector PlaneNormal = FVector::UpVector;
		FVector LineEnd = WorldLoc + WorldDir * 10000.f;

		// 1. 평행 체크 (분모가 0이 되는지 확인)
		// 방향 벡터와 평면 법선 벡터의 내적을 구합니다.
		float Denominator = FVector::DotProduct(WorldDir, PlaneNormal);

		// 분모가 0에 가깝지 않을 때만 (즉, 평면을 향하고 있을 때만) 함수 호출
		if (FMath::Abs(Denominator) > KINDA_SMALL_NUMBER)
		{
			return FMath::LinePlaneIntersection(WorldLoc, LineEnd, PlaneOrigin, PlaneNormal);
		}
	}
	return Avatar->GetActorLocation();
}
