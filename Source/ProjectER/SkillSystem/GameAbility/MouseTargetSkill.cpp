// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameAbility/MouseTargetSkill.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "SkillSystem/GameplayAbilityTargetActor/TargetActor.h"
#include "Kismet/KismetMathLibrary.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "MouseClickSkill.h"

#define ECC_SKill ECC_GameTraceChannel6

UMouseTargetSkill::UMouseTargetSkill()
{

}

void UMouseTargetSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	SetWaitTargetTask();

	//if (IsLocallyControlled())
	//{
	//	SetWaitTargetTask();
	//	/*if (CastInstantly() == false)
	//	{
	//		UE_LOG(LogTemp, Warning, TEXT("SetWaitTargetTask"));
	//		SetWaitTargetTask();
	//	}*/
	//}
}

void UMouseTargetSkill::ExecuteSkill()
{
	Super::ExecuteSkill();
	UMouseTargetSkillConfig* Config = Cast<UMouseTargetSkillConfig>(CachedConfig);
	if (!Config || AffectedActors.Num() <= 0) return;
	const TArray<TObjectPtr<USkillEffectDataAsset>>& EffectDataAssets = Config->GetEffectsToApply();

	ApplyEffectsToActors(AffectedActors, EffectDataAssets);
	RotateToTarget(AffectedActors.begin()->Get());
	//FinishSkill();
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
	ATargetActor* MyTargetActor = nullptr;
	if (WaitTargetTask->BeginSpawningActor(this, ATargetActor::StaticClass(), SpawnedActor))
	{
		MyTargetActor = Cast<ATargetActor>(SpawnedActor);
		if (MyTargetActor)
		{
			MyTargetActor->PrimaryPC = Cast<APlayerController>(GetActorInfo().PlayerController);
			WaitTargetTask->FinishSpawningActor(this, SpawnedActor);
		}
	}

	WaitTargetTask->ReadyForActivation();

	if (IsLocallyControlled())
	{
		if (IsValid(MyTargetActor)) {
			MyTargetActor->TryConfirmMouseTarget();
		}
	}
}

void UMouseTargetSkill::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle)
{
	TArray<AActor*> TargetActors = UAbilitySystemBlueprintLibrary::GetActorsFromTargetData(DataHandle, 0);

	if (TargetActors.Num() <= 0)
	{
		return;
	}

	for (AActor* Actor : TargetActors)
	{
		AffectedActors.Add(Actor);
	}

	PrepareToActiveSkill();
}

void UMouseTargetSkill::OnTargetCancelled(const FGameplayAbilityTargetDataHandle& DataHandle)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

AActor* UMouseTargetSkill::GetTargetUnderCursorInRange()
{
	if (IsLocallyControlled() == false) {
		return nullptr;
	}

	AActor* HitActor = GetTargetUnderCursor();

	if (!IsValid(HitActor)) return nullptr;

	if (IsInRange(HitActor) && IsValidRelationship(HitActor))
	{
		return HitActor;
	}

	return nullptr;
}

AActor* UMouseTargetSkill::GetTargetUnderCursor()
{
	APlayerController* PC = Cast<APlayerController>(GetActorInfo().PlayerController.Get());
	if (!PC) return nullptr;

	FHitResult HitResult;
	PC->GetHitResultUnderCursor(ECC_SKill, false, HitResult);

	return HitResult.GetActor();
}

bool UMouseTargetSkill::IsInRange(AActor* Actor)
{
	if (!IsValid(Actor)) return false;

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!IsValid(Avatar)) return false;

	UMouseTargetSkillConfig* Config = Cast<UMouseTargetSkillConfig>(CachedConfig);
	checkf(IsValid(Config), TEXT("UMouseTargetSkill::IsInRange - Config Is Not Valid"));

	FVector TargetLocation = Actor->GetActorLocation();
	FVector InstigatorLocation = Avatar->GetActorLocation();

	float DistanceSquared = FVector::DistSquaredXY(TargetLocation, InstigatorLocation);

	float RangeWithBuffer = Config->GetRange();

	if (DistanceSquared <= FMath::Square(RangeWithBuffer))
	{
		return true;
	}

	return false;
}

void UMouseTargetSkill::RotateToTarget(AActor* Actor)
{
	// 1. 유효성 체크
	if (!IsValid(Actor)) return;

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!IsValid(Avatar)) return;

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(Avatar);
	if (!IsValid(BaseCharacter)) return;
	BaseCharacter->StopMove();

	FVector StartLocation = Avatar->GetActorLocation();
	FVector TargetLocation = Actor->GetActorLocation();

	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, TargetLocation);

	// 4. 수평 회전(Yaw)만 적용 (캐릭터가 위아래로 기울어지는 것 방지)
	FRotator NewRotation = Avatar->GetActorRotation();
	NewRotation.Yaw = LookAtRotation.Yaw;

	Avatar->SetActorRotation(NewRotation);

	//컨트롤러값도 수정 이후 확인 필요
	/*if (APlayerController* PC = Cast<APlayerController>(GetActorInfo().PlayerController.Get()))
	{
		PC->SetControlRotation(NewRotation);
	}*/
}
