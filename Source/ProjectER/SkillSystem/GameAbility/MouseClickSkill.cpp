// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameAbility/MouseClickSkill.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "Monster/BaseMonster.h"
#include "SkillSystem/GameplayAbilityTargetActor/MouseLocationTargetActor.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "Kismet/KismetMathLibrary.h"

UMouseClickSkill::UMouseClickSkill()
{
	ExternalTargetLocationEventTag = FGameplayTag::RequestGameplayTag(FName("Skill.Data.Location"));
}

void UMouseClickSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	SetWaitExternalTargetEventTask();
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

bool UMouseClickSkill::IsInRange(const FVector& Location) const
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (IsValid(Avatar) == false) {
		//UE_LOG(LogTemp, Warning, TEXT("IsInRange ::IsValid(Avatar) == false"));
		return false;
	}

	UMouseClickSkillConfig* Config = Cast<UMouseClickSkillConfig>(CachedConfig);
	if (IsValid(Config) == false) {
		//UE_LOG(LogTemp, Warning, TEXT("IsInRange ::IsValid(Config) == false"));
		return false;
	}

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

void UMouseClickSkill::ExecuteSkill()
{
	Super::ExecuteSkill();

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UMouseClickSkill::SetWaitExternalTargetEventTask()
{
	if (!ExternalTargetLocationEventTag.IsValid()) return;

	UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, ExternalTargetLocationEventTag, nullptr, false, true);
	if (!IsValid(WaitEventTask)) return;

	WaitEventTask->EventReceived.AddDynamic(this, &UMouseClickSkill::OnExternalTargetLocationReceived);
	WaitEventTask->ReadyForActivation();
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
			CurrentMouseLocationTargetActor = MouseLocationTargetActor;
			MouseLocationTargetActor->PrimaryPC = Cast<APlayerController>(GetActorInfo().PlayerController);
			WaitTargetTask->FinishSpawningActor(this, SpawnedActor);
		}
	}

	WaitTargetTask->ReadyForActivation();

	FVector PendingLocation = FVector::ZeroVector;
	if (ConsumePendingExternalTargetLocation(PendingLocation))
	{
		MouseLocationTargetActor->SubmitExternalLocation(PendingLocation);
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetActorInfo().PlayerController.Get());
	const bool bCanUseMouseConfirm = IsLocallyControlled() && IsValid(PlayerController) && PlayerController->IsLocalPlayerController();
	if (bCanUseMouseConfirm)
	{
		MouseLocationTargetActor->TryConfirmMouseLocation();
	}
}

void UMouseClickSkill::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle) 
{
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
	CurrentMouseLocationTargetActor = nullptr;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UMouseClickSkill::OnExternalTargetLocationReceived(FGameplayEventData Payload)
{
	if (!Payload.TargetData.IsValid(0)) {
		UE_LOG(LogTemp, Warning, TEXT("OnExternalTargetLocationReceived::false"));
		return;
	}

	const FVector TargetLocation = Payload.TargetData.Get(0)->GetEndPoint();
	SubmitExternalTargetLocation(TargetLocation);
}

void UMouseClickSkill::SubmitExternalTargetLocation(const FVector& InLocation)
{
	if (!IsInRange(InLocation))
	{
		//UE_LOG(LogTemp, Warning, TEXT("SubmitExternalTargetLocation::false"));
		return;
	}

	if (CurrentMouseLocationTargetActor.IsValid())
	{
		//UE_LOG(LogTemp, Warning, TEXT("CurrentMouseLocationTargetActor.IsValid()"));
		CurrentMouseLocationTargetActor->SubmitExternalLocation(InLocation);
		return;
	}
	else {
		//UE_LOG(LogTemp, Warning, TEXT("CurrentMouseLocationTargetActor.IsNotValid()"));
	}

	PendingExternalTargetLocation = InLocation;
}

bool UMouseClickSkill::ConsumePendingExternalTargetLocation(FVector& OutLocation)
{
	if (!PendingExternalTargetLocation.IsSet()) return false;

	OutLocation = PendingExternalTargetLocation.GetValue();
	PendingExternalTargetLocation.Reset();
	return true;
}

bool UMouseClickSkill::IsTargetLocationInRange(const FVector& InLocation) const
{
	return IsInRange(InLocation);
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
