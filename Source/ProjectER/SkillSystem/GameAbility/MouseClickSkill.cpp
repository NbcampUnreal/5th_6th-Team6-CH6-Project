// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameAbility/MouseClickSkill.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "AbilitySystemComponent.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "Monster/BaseMonster.h"
#include "SkillSystem/GameplayAbilityTargetActor/MouseLocationTargetActor.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "Kismet/KismetMathLibrary.h"

void UMouseClickSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	//PrepareDataSetDelegate(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	/*AActor* Avatar = GetAvatarActorFromActorInfo();
	if (IsValid(Avatar) == false) return;
	ABaseMonster* IsMonster = Cast<ABaseMonster>(Avatar);
	if (IsValid(IsMonster))
	{
		SendLocationData(IsMonster->GetActorLocation());
	}
	else {
		SendLocationData(GetMouseLocation());
	}*/

	SetWaitTargetTask();
}

bool UMouseClickSkill::TryGetMouseLocationInRange(FVector& OutLocation) const
{
	if (!IsLocallyControlled()) return false;

	const FVector TargetLocation = GetMouseLocation();
	if (!IsInRange(TargetLocation)) return false;

	OutLocation = TargetLocation;
	return true;
}

//void UMouseClickSkill::SendLocationData(FVector TargetLocation)
//{
//	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
//	if (!ASC) return;
//
//	FGameplayAbilityTargetDataHandle DataHandle;
//	FGameplayAbilityTargetData_LocationInfo* LocData = new FGameplayAbilityTargetData_LocationInfo();
//	LocData->TargetLocation.LiteralTransform = FTransform(TargetLocation);
//	DataHandle.Add(LocData);
//
//	FScopedPredictionWindow ScopedPrediction(ASC, CurrentActorInfo->IsLocallyControlled());
//	ASC->CallServerSetReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey(), DataHandle, FGameplayTag(), ASC->ScopedPredictionKey);
//}

bool UMouseClickSkill::IsInRange(const FVector& Location) const
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (IsValid(Avatar) == false) return false;

	UMouseClickSkillConfig* Config = Cast<UMouseClickSkillConfig>(CachedConfig);
	if (IsValid(Config) == false) return false;

	const FVector InstigatorLocation = Avatar->GetActorLocation();
	const float DistanceSquared = FVector::DistSquaredXY(Location, InstigatorLocation);
	const float RangeWithBuffer = Config->GetRange();
	
	return DistanceSquared <= FMath::Square(RangeWithBuffer);
}

void UMouseClickSkill::RotateToLocation(const FVector& Location)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!IsValid(Avatar)) return;

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(Avatar);
	if (IsValid(BaseCharacter))
	{
		BaseCharacter->StopMove();
	}

	const FVector StartLocation = Avatar->GetActorLocation();
	const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, Location);

	FRotator NewRotation = Avatar->GetActorRotation();
	NewRotation.Yaw = LookAtRotation.Yaw;

	Avatar->SetActorRotation(NewRotation);
}

//void UMouseClickSkill::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag)
//{
//	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
//	check(ASC);
//	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo)) return;
//
//	FinalLocation = DataHandle.Get(0)->GetEndPoint();
//
//	PrepareToActiveSkill();
//
//	// 마무리 작업
//	ASC->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey()).Remove(TargetDataDelegateHandle);
//	ASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
//}

void UMouseClickSkill::ExecuteSkill()
{
	Super::ExecuteSkill();

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UMouseClickSkill::SetWaitTargetTask()
{
	UAbilityTask_WaitTargetData* WaitTargetTask = UAbilityTask_WaitTargetData::WaitTargetData(
		this,
		TEXT("WaitMouseLocationTargetTask"),
		EGameplayTargetingConfirmation::UserConfirmed,
		AMouseLocationTargetActor::StaticClass()
	);

	WaitTargetTask->ValidData.AddDynamic(this, &UMouseClickSkill::OnTargetDataReady);
	WaitTargetTask->Cancelled.AddDynamic(this, &UMouseClickSkill::OnTargetCancelled);

	AGameplayAbilityTargetActor* SpawnedActor = nullptr;
	AMouseLocationTargetActor* MouseLocationTargetActor = nullptr;
	if (WaitTargetTask->BeginSpawningActor(this, AMouseLocationTargetActor::StaticClass(), SpawnedActor))
	{
		MouseLocationTargetActor = Cast<AMouseLocationTargetActor>(SpawnedActor);
		if (MouseLocationTargetActor)
		{
			MouseLocationTargetActor->PrimaryPC = Cast<APlayerController>(GetActorInfo().PlayerController);
			WaitTargetTask->FinishSpawningActor(this, SpawnedActor);
		}
	}

	WaitTargetTask->ReadyForActivation();

	if (IsLocallyControlled() && IsValid(MouseLocationTargetActor))
	{
		MouseLocationTargetActor->TryConfirmMouseLocation();
	}
	
}

void UMouseClickSkill::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle) 
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	check(ASC);
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo)) return;
	if (!DataHandle.IsValid(0) || !CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo)) return;

	FVector Location(DataHandle.Get(0)->GetEndPoint());

	if (IsInRange(Location) == false)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	RotateToLocation(Location);
	PrepareToActiveSkill();
}

void UMouseClickSkill::OnTargetCancelled(const FGameplayAbilityTargetDataHandle& DataHandle)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

//void UMouseClickSkill::PrepareDataSetDelegate(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
//{
//	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
//	check(ASC);
//
//	TargetDataDelegateHandle = ASC->AbilityTargetDataSetDelegate(Handle, ActivationInfo.GetActivationPredictionKey()).AddUObject(this, &UMouseClickSkill::OnTargetDataReady);
//	ASC->CallReplicatedTargetDataDelegatesIfSet(Handle, ActivationInfo.GetActivationPredictionKey());
//}

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
