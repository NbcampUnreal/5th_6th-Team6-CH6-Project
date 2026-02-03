// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/AGameplayAbilityTargetActor/TargetActor.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemBlueprintLibrary.h"

ATargetActor::ATargetActor()
{
    //bDestroyOnConfirmation = true;
}

void ATargetActor::BeginPlay()
{
    Super::BeginPlay();
}

void ATargetActor::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);
}

void ATargetActor::ConfirmTargetingAndContinue()
{
    if (!IsValid(PrimaryPC))
    {
        return;
    }

    FHitResult HitResult;
    if (PrimaryPC->GetHitResultUnderCursor(ECC_Pawn, false, HitResult))
    {
        AActor* HitActor = HitResult.GetActor();

        if (IsValid(HitActor))
        {
            UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);

            if (TargetASC)
            {
                UE_LOG(LogTemp, Log, TEXT("TargetActor: ASC를 가진 유효한 타겟 발견! (%s)"), *HitActor->GetName());

                FGameplayAbilityTargetDataHandle Handle = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(HitActor);
                TargetDataReadyDelegate.Broadcast(Handle);
                return;
            }
        }
    }

    FGameplayAbilityTargetDataHandle CancelHandle;
    CanceledDelegate.Broadcast(CancelHandle);
}
