// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameAbility/MouseTargetSkill.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"

UMouseTargetSkill::UMouseTargetSkill()
{

}

void UMouseTargetSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	Targeted();
}

void UMouseTargetSkill::ExecuteSkill()
{
	UMouseTargetSkillConfig* Config = Cast<UMouseTargetSkillConfig>(SkillConfig);
	if (!Config) return;

	const TArray<FSkillEffectData>& Effects = Config->GetEffects();

	// 타겟 획득 로직 (생략)...

	for (AActor* TargetActor : AffectedActors)
	{
		if (!IsValid(TargetActor)) continue;

		// 1. 타겟의 ASC(AbilitySystemComponent) 가져오기
		UAbilitySystemComponent* TargetASC = nullptr;

		if (IAbilitySystemInterface* ASInterface = Cast<IAbilitySystemInterface>(TargetActor))
		{
			TargetASC = ASInterface->GetAbilitySystemComponent();
		}

		if (!TargetASC) TargetASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();

		FGameplayAbilityTargetDataHandle TargetDataHandle;
		FGameplayAbilityTargetData_ActorArray* NewData = new FGameplayAbilityTargetData_ActorArray();
		NewData->TargetActorArray.Add(TargetActor);
		TargetDataHandle.Add(NewData);

		if (TargetASC)
		{
			for (const FSkillEffectData& EffectData : Effects)
			{
				FGameplayEffectContextHandle ContextHandle = TargetASC->MakeEffectContext();
				FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(EffectData.SkillEffectClass, GetAbilityLevel(), ContextHandle);

				FGameplayTag DataTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage.Magnitude"));
				//SpecHandle.Data.Get()->SetSetByCallerMagnitude(DataTag, FinalValue);
				
				// 3. 타겟에게 적용
			

				if (SpecHandle.IsValid())
				{
					ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle, TargetDataHandle);
				}
			}
		}
	}
}

void UMouseTargetSkill::Targeted()
{
	//1. 입력을 기다립니다. (사용자가 왼쪽 마우스 버튼을 누를 때까지)
	// bTestInputAlreadyPressed를 false로 해서, 함수 호출 이전에 누르고 있던 건 무시합니다.
	UAbilityTask_WaitInputPress* WaitInputTask = UAbilityTask_WaitInputPress::WaitInputPress(this, false);

	if (WaitInputTask)
	{
		// 2. 클릭했을 때 실행될 함수 연결
		WaitInputTask->OnPress.AddDynamic(this, &UMouseTargetSkill::OnTargetConfirmed);
		WaitInputTask->ReadyForActivation();
		UE_LOG(LogTemp, Log, TEXT("타겟팅 모드 진입: 적을 클릭하세요."));
	}
}

void UMouseTargetSkill::OnTargetConfirmed(float ElapsedTime)
{
	//ElapsedTime는 빌리티가 실행된 순간부터 ~ 실제 키가 눌린 순간까지의 시간 차이인데 안씁니다.
	APlayerController* PC = GetActorInfo().PlayerController.Get();
	if (!PC) return;

	FHitResult HitResult;
	if (PC->GetHitResultUnderCursor(ECC_Pawn, false, HitResult)) {
		AActor* HitActor = HitResult.GetActor();
		if (HitActor)
		{
			float Distance = FVector::Dist(GetAvatarActorFromActorInfo()->GetActorLocation(), HitActor->GetActorLocation());

			UMouseTargetSkillConfig* Config = Cast<UMouseTargetSkillConfig>(SkillConfig);
			if (Config)
			{
				float Range = Config->GetRange();
				if (Distance <= Range)
				{
					UE_LOG(LogTemp, Warning, TEXT("적 타겟팅 성공: %s"), *HitActor->GetName());
					PrepareToActiveSkill();
					AffectedActors.Add(HitActor);
					ExecuteSkill();
				}
			}
		}
	}
}
