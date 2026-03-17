// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/Actor/PeriodicOverlapPoolActor/PeriodicOverlapPoolActor.h"
#include "Components/ShapeComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "CharacterSystem/Interface/TargetableInterface.h"
#include "TimerManager.h"

APeriodicOverlapPoolActor::APeriodicOverlapPoolActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void APeriodicOverlapPoolActor::InitializePeriodicData(float InPeriod, bool bInApplyImmediately)
{
    Period = InPeriod;
    bApplyImmediately = bInApplyImmediately;
}

void APeriodicOverlapPoolActor::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        // 부모 클래스의 Overlap 이벤트는 이미 등록되어 있음 (BaseRangeOverlapEffectActor::BeginPlay)
        // 하지만 EndOverlap은 추가로 등록 필요
        if (IsValid(CollisionComponent))
        {
            CollisionComponent->OnComponentEndOverlap.AddDynamic(this, &APeriodicOverlapPoolActor::OnShapeEndOverlap);
        }

        // 타이머 시작
        if (Period > 0.f)
        {
            GetWorldTimerManager().SetTimer(PeriodicTimerHandle, this, &APeriodicOverlapPoolActor::ApplyPeriodicEffects, Period, true, bApplyImmediately ? 0.f : Period);
        }
    }
}

void APeriodicOverlapPoolActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GetWorldTimerManager().IsTimerActive(PeriodicTimerHandle))
    {
        GetWorldTimerManager().ClearTimer(PeriodicTimerHandle);
    }

    Super::EndPlay(EndPlayReason);
}

void APeriodicOverlapPoolActor::OnShapeBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority() || !IsValid(OtherActor) || OtherActor == this || OtherActor == InstigatorActor)
    {
        return;
    }

    // 팀 확인 필터링 (Base 클래스 로직 활용)
    if (ITargetableInterface* MyInstigatorTargetable = Cast<ITargetableInterface>(InstigatorActor))
    {
        if (ITargetableInterface* OtherTargetable = Cast<ITargetableInterface>(OtherActor))
        {
            if (MyInstigatorTargetable->GetTeamType() == OtherTargetable->GetTeamType())
            {
                return;
            }
        }
    }

    // 현재 관리 중인 목록에 추가
    CurrentOverlappingActors.Add(OtherActor);

    // 즉시 적용이 아니고, 타이머가 돌고 있다면 다음 주기에 적용됨
    // 만약 진입 즉시 1회 적용이 필요하다면 여기서 ApplyEffectsToTarget 호출 가능
}

void APeriodicOverlapPoolActor::OnShapeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (HasAuthority() && IsValid(OtherActor))
    {
        CurrentOverlappingActors.Remove(OtherActor);
    }
}

void APeriodicOverlapPoolActor::ApplyPeriodicEffects()
{
    if (!HasAuthority())
    {
        return;
    }

    // 유효하지 않은 액터 제거 및 정리
    TArray<TObjectPtr<AActor>> ToRemove;
    for (auto It = CurrentOverlappingActors.CreateIterator(); It; ++It)
    {
        AActor* Actor = It->Get();
        if (!IsValid(Actor))
        {
            It.RemoveCurrent();
            continue;
        }

        ApplyEffectsToTarget(Actor);
    }
}

void APeriodicOverlapPoolActor::ApplyEffectsToTarget(AActor* TargetActor)
{
    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
    UAbilitySystemComponent* const InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorActor);
    if (!IsValid(TargetASC) || EffectSpecHandles.Num() <= 0)
    {
        return;
    }

    // 이펙트 적용
    for (const FGameplayEffectSpecHandle& EffectSpecHandle : EffectSpecHandles)
    {
        if (EffectSpecHandle.IsValid())
        {
            InstigatorASC->ApplyGameplayEffectSpecToTarget(*EffectSpecHandle.Data.Get(), TargetASC);
        }
    }

    // 시각 효과(VFX) 실행
    if (IsValid(InstigatorASC) && HitTargetCueParameters.OriginalTag.IsValid())
    {
        FGameplayCueParameters CueParameters = HitTargetCueParameters;
        CueParameters.Location = TargetActor->GetActorLocation();
        CueParameters.EffectCauser = this;
        CueParameters.SourceObject = HitTargetCueSourceObject;
        
        FScopedPredictionWindow ForcedWindow(InstigatorASC, FPredictionKey(), false);
        InstigatorASC->ExecuteGameplayCue(CueParameters.OriginalTag, CueParameters);
    }
}
