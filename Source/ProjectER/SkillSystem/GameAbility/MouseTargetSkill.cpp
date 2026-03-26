// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameAbility/MouseTargetSkill.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "SkillSystem/GameplayEffect/SkillEffectDataAsset.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "SkillSystem/GameplayAbilityTargetActor/TargetActor.h"
#include "Kismet/KismetMathLibrary.h"
#include "CharacterSystem/Character/BaseCharacter.h"

#define ECC_SKill ECC_GameTraceChannel6

UMouseTargetSkill::UMouseTargetSkill()
{
	ExternalTargetActorEventTag = FGameplayTag::RequestGameplayTag(FName("Skill.Data.Target"));
}

void UMouseTargetSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	SetWaitExternalTargetEventTask();
	SetWaitTargetTask();
}

void UMouseTargetSkill::ExecuteSkill()
{
	Super::ExecuteSkill();

	AActor* const TargetActor = AffectedActor.Get();
	if (!IsValid(TargetActor)) return;

	UMouseTargetSkillConfig* Config = Cast<UMouseTargetSkillConfig>(CachedConfig);
	if (!IsValid(Config)) return;

	const TArray<TObjectPtr<USkillEffectDataAsset>>& EffectDataAssets = Config->GetEffectsToApply();
	if (EffectDataAssets.Num() <= 0) return;
	ApplyEffectsTarget(TargetActor, EffectDataAssets);
}

void UMouseTargetSkill::CompleteFinishSkill()
{
	CleanUpSkill();
	Super::CompleteFinishSkill();
}

void UMouseTargetSkill::OnCancelAbility()
{
	CleanUpSkill();
	Super::OnCancelAbility();
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
			CurrentTargetActor = MyTargetActor;
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

void UMouseTargetSkill::SetWaitExternalTargetEventTask()
{
	if (!ExternalTargetActorEventTag.IsValid()) return;

	UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, ExternalTargetActorEventTag, nullptr, false, true);
	if (!IsValid(WaitEventTask)) return;

	WaitEventTask->EventReceived.AddDynamic(this, &UMouseTargetSkill::OnExternalTargetActorReceived);
	WaitEventTask->ReadyForActivation();
}

void UMouseTargetSkill::SubmitExternalTargetActor(AActor* InTargetActor)
{
	if (!IsTargetActorInRange(InTargetActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("SubmitExternalTargetActor::OutOfRange"));
		return;
	}

	if (CurrentTargetActor.IsValid())
	{
		CurrentTargetActor->SubmitExternalTarget(InTargetActor);
		return;
	}

	PendingExternalTargetActor = InTargetActor;
}

bool UMouseTargetSkill::ConsumePendingExternalTargetActor(AActor*& OutTargetActor)
{
	if (!PendingExternalTargetActor.IsValid()) return false;

	OutTargetActor = PendingExternalTargetActor.Get();
	PendingExternalTargetActor = nullptr;
	return true;
}


void UMouseTargetSkill::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle)
{
	TArray<AActor*> TargetActors = UAbilitySystemBlueprintLibrary::GetActorsFromTargetData(DataHandle, 0);

	if (TargetActors.Num() <= 0)
	{
		return;
	}

	AffectedActor = nullptr;
	for (AActor* const Actor : TargetActors)
	{
		if (!IsValid(Actor)) continue;
		AffectedActor = Actor;
		RotateToTarget(Actor);
		break;
	}

	if (!AffectedActor.IsValid()) return;

	PrepareToActiveSkill();
}

void UMouseTargetSkill::OnTargetCancelled(const FGameplayAbilityTargetDataHandle& DataHandle)
{
	CurrentTargetActor = nullptr;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UMouseTargetSkill::OnExternalTargetActorReceived(FGameplayEventData Payload)
{
	const AActor* Target = Payload.Target;

	if (IsValid(Target))
	{
		AActor* NonConstTarget = const_cast<AActor*>(Target);
		SubmitExternalTargetActor(NonConstTarget);
	}
}

AActor* UMouseTargetSkill::GetTargetUnderCursorInRange()
{
	if (IsLocallyControlled() == false) {
		return nullptr;
	}

	AActor* HitActor = GetTargetUnderCursor();

	if (!IsValid(HitActor)) return nullptr;

	if (IsTargetActorInRange(HitActor))
	{
		return HitActor;
	}

	return nullptr;
}

bool UMouseTargetSkill::IsTargetActorInRange(AActor* InTargetActor)
{
	return IsInRange(InTargetActor) && IsValidRelationship(InTargetActor);
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
	if (!ensureMsgf(IsValid(Config), TEXT("UMouseTargetSkill::IsInRange - Config Is Not Valid"))) { return false; }

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
}

void UMouseTargetSkill::ApplyEffectsTarget(AActor* TargetActor, const TArray<TObjectPtr<USkillEffectDataAsset>>& SkillEffectDataAssets)
{
	UAbilitySystemComponent* const SourceASC = GetAbilitySystemComponentFromActorInfo();
	AActor* const Avatar = GetAvatarActorFromActorInfo();
	if (!IsValid(SourceASC) || !IsValid(Avatar) || !IsValid(TargetActor) || SkillEffectDataAssets.Num() <= 0) return;
	if (!IsValidRelationship(TargetActor)) return;

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.AddInstigator(Avatar, Avatar);
	ContextHandle.SetAbility(this);
	ContextHandle.AddOrigin(TargetActor->GetActorLocation());
	FHitResult HitResult(TargetActor, nullptr, TargetActor->GetActorLocation(), FVector::UpVector);
	ContextHandle.AddHitResult(HitResult);

	UAbilitySystemComponent* const TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!IsValid(TargetASC)) return;

	for (USkillEffectDataAsset* const Effect : SkillEffectDataAssets)
	{
		if (!IsValid(Effect)) continue;

		for (FGameplayEffectSpecHandle& Spec : Effect->MakeSpecs(SourceASC, this, Avatar, ContextHandle))
		{
			if (!Spec.IsValid() || !Spec.Data.IsValid()) continue;
			SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
		}
	}
}

void UMouseTargetSkill::CleanUpSkill()
{
	CurrentTargetActor = nullptr;
	AffectedActor = nullptr;
}
