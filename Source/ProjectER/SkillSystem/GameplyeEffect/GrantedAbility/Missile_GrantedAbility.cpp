// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplyeEffect/GrantedAbility/Missile_GrantedAbility.h"
#include "GameplayAbilitySpec.h"
#include "AbilitySystemComponent.h"

UMissile_GrantedAbility::UMissile_GrantedAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
}

void UMissile_GrantedAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    UE_LOG(LogTemp, Warning, TEXT("ActivateAbility"));
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    AActor* TargetActor = GetAvatarActorFromActorInfo();
    AActor* Instigator = nullptr;

    // 1. 현재 어빌리티의 Spec을 가져옵니다.
    FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();

    if (Spec && ActorInfo->AbilitySystemComponent.IsValid())
    {
        // 2. 이 어빌리티를 부여한 Gameplay Effect의 핸들을 확인합니다.
        FActiveGameplayEffectHandle GEHandle = Spec->GameplayEffectHandle;

        if (GEHandle.IsValid())
        {
            // 3. ASC를 통해 해당 GE의 실제 데이터를 찾습니다.
            const FActiveGameplayEffect* ActiveGE = ActorInfo->AbilitySystemComponent->GetActiveGameplayEffect(GEHandle);
            if (ActiveGE)
            {
                // 4. 드디어 시전자(Instigator)를 찾았습니다!
                Instigator = ActiveGE->Spec.GetContext().GetInstigator();
            }
        }
    }

    // 5. 만약 위에서 못 찾았다면 차선책 (일반적인 시전자 찾기)
    //if (!IsValid(Instigator))
    //{
    //    Instigator = GetInstigatorFromActorInfo();
    //}

    //// ---------------------------------------------------------
    //// 6. 미사일 발사 로직
    //if (IsValid(Instigator) && IsValid(TargetActor))
    //{
    //    FVector SpawnLocation = Instigator->GetActorLocation();

    //    if (MissileClass)
    //    {
    //        AMyProjectile* Missile = GetWorld()->SpawnActorDeferred<AMyProjectile>(
    //            MissileClass, FTransform(Instigator->GetActorRotation(), SpawnLocation));

    //        if (Missile)
    //        {
    //            Missile->InitializeMissile(TargetActor);
    //            Missile->FinishSpawning(FTransform(Instigator->GetActorRotation(), SpawnLocation));
    //        }
    //    }
    //}

    //EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UMissile_GrantedAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
    // 1. 부모 함수 호출 (필수)
    Super::OnAvatarSet(ActorInfo, Spec);
    UE_LOG(LogTemp, Warning, TEXT("OnAvatarSet"));
    // 2. 서버에서만 실행되도록 체크 (어빌리티 실행은 보통 서버 주도)
    if (!Spec.IsActive())
    {
        // TryActivateAbility를 호출하여 자신을 깨웁니다.
        ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
    }
}