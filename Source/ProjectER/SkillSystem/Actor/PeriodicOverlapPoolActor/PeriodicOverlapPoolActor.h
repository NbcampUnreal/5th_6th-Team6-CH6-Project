// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/Actor/BaseRangeOverlapEffectActor/BaseRangeOverlapEffectActor.h"
#include "PeriodicOverlapPoolActor.generated.h"

/**
 * 일정 주기마다 범위 내 타겟에게 Gameplay Effect를 적용하는 액터입니다.
 */
UCLASS()
class PROJECTER_API APeriodicOverlapPoolActor : public ABaseRangeOverlapEffectActor
{
    GENERATED_BODY()

public:
    APeriodicOverlapPoolActor();

    /**
     * 주기적 효과 데이터를 초기화합니다.
     */
    void InitializePeriodicData(float InPeriod, bool bInApplyImmediately);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // ABaseRangeOverlapEffectActor의 Overlap 로직 확장
    virtual void OnShapeBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
    
    UFUNCTION()
    virtual void OnShapeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    /**
     * 주기적으로 실행될 효과 적용 함수
     */
    virtual void ApplyPeriodicEffects();

    /**
     * 특정 타겟에게 효과를 적용합니다 (CDO나 GEC에서 설정된 Spec 기반)
     */
    void ApplyEffectsToTarget(AActor* TargetActor);

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Periodic")
    float Period = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Periodic")
    bool bApplyImmediately = true;

    /** 현재 범위 내에 있는 유효한 타겟 목록 */
    UPROPERTY()
    TSet<TObjectPtr<AActor>> CurrentOverlappingActors;

    FTimerHandle PeriodicTimerHandle;
};
