// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "SkillSystem/SkillNiagaraSpawnSettings.h"
#include "BaseRangeOverlapEffectActor.generated.h"

class UShapeComponent;
class USphereComponent;
class UBoxComponent;
class UCapsuleComponent;
class UAreaPeriodicEffectComponent;

UCLASS()
class PROJECTER_API ABaseRangeOverlapEffectActor : public AActor {
  GENERATED_BODY()

public:
  // Sets default values for this actor's properties
  ABaseRangeOverlapEffectActor();

  void InitializeEffectData(
      const TArray<FGameplayEffectSpecHandle> &InEffectSpecHandles,
      AActor *InInstigatorActor, const FVector &InCollisionSize,
      bool bInHitOncePerTarget, const UObject *InHitTargetCueSourceObject,
      const FGameplayCueParameters &InHitTargetCueParameters);

  /** 컴포넌트를 이 액터의 도트 관리자로 설정 (GEC에서 호출) */
  void SetAreaPeriodicComponent(UAreaPeriodicEffectComponent* InComponent);

protected:
  virtual void BeginPlay() override;
  virtual void ApplyCollisionSize(const FVector &InCollisionSize);
  void SetCollisionComponent(UShapeComponent *InCollisionComponent);

  UFUNCTION()
  virtual void OnShapeBeginOverlap(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult);

  UFUNCTION()
  virtual void OnShapeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

  /** 주기적 트리거 발생 시 호출되는 로직 */
  UFUNCTION()
  virtual void OnAreaPeriodicTrigger(const TArray<AActor*>& Targets);

  /** 타겟들에게 효과 적용 */
  void ApplyEffectsToTargets(const TArray<AActor*>& Targets);
  void ApplyEffectsToTarget(AActor* TargetActor);

public:
  UPROPERTY()
  bool bDestroyOnOverlap = false;

protected:
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
  TObjectPtr<UShapeComponent> CollisionComponent;

  UPROPERTY()
  TArray<FGameplayEffectSpecHandle> EffectSpecHandles;

  UPROPERTY()
  TObjectPtr<AActor> InstigatorActor;

  UPROPERTY()
  bool bHitOncePerTarget = true;

  UPROPERTY()
  FVector PendingCollisionSize = FVector::ZeroVector;

  UPROPERTY()
  bool bHasPendingCollisionSize = false;

  UPROPERTY()
  TSet<TObjectPtr<AActor>> HitActors;

  UPROPERTY()
  TObjectPtr<const UObject> HitTargetCueSourceObject;

  UPROPERTY()
  FGameplayCueParameters HitTargetCueParameters;

  /** 도트 로직용 컴포넌트 */
  UPROPERTY()
  TObjectPtr<UAreaPeriodicEffectComponent> AreaPeriodicComponent;
};
