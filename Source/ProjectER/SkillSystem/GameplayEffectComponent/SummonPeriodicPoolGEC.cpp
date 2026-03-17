// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffectComponent/SummonPeriodicPoolGEC.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeGEC.h"
#include "SkillSystem/Actor/BaseRangeOverlapEffectActor/BaseRangeOverlapEffectActor.h"
#include "SkillSystem/Component/AreaPeriodicEffectComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "Components/SkeletalMeshComponent.h"

USummonPeriodicPoolGEC::USummonPeriodicPoolGEC()
{
    ConfigClass = USummonPeriodicPoolConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> USummonPeriodicPoolGEC::GetRequiredConfigClass() const
{
    return USummonPeriodicPoolConfig::StaticClass();
}

FTransform USummonPeriodicPoolGEC::CalculateSpawnTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const AActor* TargetActor) const
{
    const USummonPeriodicPoolConfig* const PeriodicConfig = ResolveTypedConfigFromSpec<USummonPeriodicPoolConfig>(GESpec);
    if (!IsValid(PeriodicConfig) || !IsValid(Instigator))
    {
        return FTransform::Identity;
    }

    FTransform SpawnTransform = FTransform::Identity;
    if (PeriodicConfig->OriginType == ESummonOriginType::InstigatorBone)
    {
        // 시전자의 본 위치 사용
        if (const USkeletalMeshComponent* const Mesh = Instigator->FindComponentByClass<USkeletalMeshComponent>())
        {
            if (Mesh->DoesSocketExist(PeriodicConfig->SummonBoneName))
            {
                SpawnTransform.SetLocation(Mesh->GetSocketLocation(PeriodicConfig->SummonBoneName));
                SpawnTransform.SetRotation(Mesh->GetSocketRotation(PeriodicConfig->SummonBoneName).Quaternion());
            }
        }

        // 본을 찾지 못했으면 액터 기본 트랜스폼 사용
        if (SpawnTransform.GetLocation().IsZero())
        {
            SpawnTransform = Instigator->GetActorTransform();
        }
    }
    
    // 만약 Context 타입이거나 위에서 위치를 찾지 못한 경우 부모의 방식(SummonRangeGEC) 사용
    if (SpawnTransform.GetLocation().IsZero())
    {
        return Super::CalculateSpawnTransform(GESpec, Instigator, TargetActor);
    }
    
    // 위치 기반인 경우 공통 옵션 적용
    FVector TargetLocation = SpawnTransform.GetLocation();
    FRotator CombinedRotation = SpawnTransform.Rotator();
    
    ApplyCommonSpawnOptions(TargetLocation, CombinedRotation, PeriodicConfig, Instigator);
    
    return FTransform(CombinedRotation, TargetLocation);
}

void USummonPeriodicPoolGEC::InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, const USummonRangeBaseConfig* Config, AActor* Instigator, const FGameplayEffectContextHandle& Context, const FGameplayCueParameters& HitTargetCueParameters) const
{
    // 부모의 초기화 로직 (Effect Specs 설정 등) 실행
    Super::InitializeRangeActor(RangeActor, Config, Instigator, Context, HitTargetCueParameters);
    
    const USummonPeriodicPoolConfig* const PeriodicConfig = Cast<USummonPeriodicPoolConfig>(Config);
    if (IsValid(RangeActor) && IsValid(PeriodicConfig))
    {
        // 1. AreaPeriodicEffectComponent 동적 생성
        UAreaPeriodicEffectComponent* PeriodicComp = NewObject<UAreaPeriodicEffectComponent>(RangeActor, UAreaPeriodicEffectComponent::StaticClass(), TEXT("DynamicAreaPeriodicEffect"));
        if (IsValid(PeriodicComp))
        {
            PeriodicComp->CreationMethod = EComponentCreationMethod::Instance;
			PeriodicComp->SetIsReplicated(true);

            // 2. 컴포넌트 등록 및 액터 할당
            PeriodicComp->RegisterComponent();
            RangeActor->AddInstanceComponent(PeriodicComp);
            RangeActor->SetAreaPeriodicComponent(PeriodicComp);

            // 3. 주기적 효과 설정 (실행은 액터의 BeginPlay에서 담당)
            PeriodicComp->SetupPeriodicEffect(PeriodicConfig->Period, PeriodicConfig->bApplyImmediately);
        }
    }
}
