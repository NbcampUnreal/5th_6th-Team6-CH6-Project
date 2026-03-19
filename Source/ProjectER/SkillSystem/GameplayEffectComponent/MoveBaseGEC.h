// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/EngineTypes.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "SkillSystem/GameplayEffectComponent/BaseGECConfig.h"

class USkillEffectDataAsset;
class USkillNiagaraSpawnConfig;
struct FGameplayEffectSpec;

#include "MoveBaseGEC.generated.h"

// 이동 방향 결정 방식
UENUM(BlueprintType)
enum class EMoveDirectionSource : uint8
{
	Forward       UMETA(DisplayName = "캐릭터 전방"),
	TowardContext UMETA(DisplayName = "Context Origin 방향"),
	TowardTarget  UMETA(DisplayName = "Target Actor 방향"),
};

// ======================================================
// Config Base
// ======================================================

UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UMoveBaseConfig : public UBaseGECConfig
{
	GENERATED_BODY()

public:
	// --- 공통 이동 설정 ---
	UPROPERTY(EditDefaultsOnly, Category = "Move|Base")
	EMoveDirectionSource DirectionSource = EMoveDirectionSource::Forward;

	UPROPERTY(EditDefaultsOnly, Category = "Move|Base")
	float MoveDistance = 500.0f;

	// --- 안전 설정 ---
	UPROPERTY(EditDefaultsOnly, Category = "Move|Safety")
	bool bIgnoreIfRootMotion = true;

	UPROPERTY(EditDefaultsOnly, Category = "Move|Safety")
	bool bIgnoreUnitCollision = false;

	// 지면 추적 거리 고정 (에디터 수정 불가)
	UPROPERTY(VisibleDefaultsOnly, Category = "Move|Safety")
	float GroundTraceDistance = 500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Move|Safety")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	// --- Wall Hit (벽꿍) ---
	UPROPERTY(EditDefaultsOnly, Category = "Move|WallHit")
	bool bDetectWallHit = false;

	UPROPERTY(EditDefaultsOnly, Category = "Move|WallHit",
		meta = (EditCondition = "bDetectWallHit"))
	TArray<TObjectPtr<USkillEffectDataAsset>> WallHitApplied;

	// --- VFX ---
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Move|VFX")
	TObjectPtr<USkillNiagaraSpawnConfig> StartVfx;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Move|VFX")
	TObjectPtr<USkillNiagaraSpawnConfig> MovingVfx;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Move|VFX")
	TObjectPtr<USkillNiagaraSpawnConfig> EndVfx;

	// --- Animation ---
	// 활성 몽타주 속도 조정 여부
	UPROPERTY(EditDefaultsOnly, Category = "Move|Animation")
	bool bAdjustMontageRate = true;

	// 최소 PlayRate 제한
	UPROPERTY(EditDefaultsOnly, Category = "Move|Animation",
		meta = (EditCondition = "bAdjustMontageRate"))
	float MinPlayRate = 0.5f;

	// 최대 PlayRate 제한
	UPROPERTY(EditDefaultsOnly, Category = "Move|Animation",
		meta = (EditCondition = "bAdjustMontageRate"))
	float MaxPlayRate = 3.0f;
};

// ======================================================
// GEC Base
// ======================================================

UCLASS(Abstract)
class PROJECTER_API UMoveBaseGEC : public UBaseGEC
{
	GENERATED_BODY()

public:
	UMoveBaseGEC();

protected:
	virtual void OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;

	// 파생 클래스에서 이동 방식별 구현 (순수 가상)
	virtual void Execute(AActor* Instigator, const FVector& Direction, const UMoveBaseConfig* Config, const FGameplayEffectSpec& GESpec) const PURE_VIRTUAL(UMoveBaseGEC::Execute, );

	// 이동 소요 시간 반환 (애니메이션 동기화용)
	virtual float CalculateMoveDuration(const AActor* Instigator, const FVector& Direction, const UMoveBaseConfig* Config) const PURE_VIRTUAL(UMoveBaseGEC::CalculateMoveDuration, return 0.15f;);

	// 공통 유틸리티 함수
	bool IsRootMotionActive(const AActor* Actor) const;

	FVector CalculateMoveDirection(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const UMoveBaseConfig* Config) const;

	void HandleWallHit(AActor* Instigator, const FHitResult& Hit, const UMoveBaseConfig* Config, const FGameplayEffectSpec& GESpec) const;

	void SnapToGround(FVector& InOutLocation, const UMoveBaseConfig* Config, const AActor* Instigator) const;
	
	// 개별 큐 실행 헬퍼
	void ExecuteMoveCue(const USkillNiagaraSpawnConfig* VfxConfig, const FGameplayEffectSpec& GESpec, AActor* Instigator, const FVector& Location) const;

	// Moving 루핑 큐 추가/제거
	void AddMovingCue(const USkillNiagaraSpawnConfig* VfxConfig, const FGameplayEffectSpec& GESpec, AActor* Instigator) const;
	void RemoveMovingCue(const USkillNiagaraSpawnConfig* VfxConfig, AActor* Instigator) const;

	// 활성 몽타주 속도 조정
	void AdjustActiveMontageRate(ACharacter* Character, float MoveDuration, const UMoveBaseConfig* Config) const;

	// 유닛 충돌 채널 설정/복구 헬퍼
	void SetPawnCollisionIgnore(ACharacter* Character, bool bIgnore) const;
};
