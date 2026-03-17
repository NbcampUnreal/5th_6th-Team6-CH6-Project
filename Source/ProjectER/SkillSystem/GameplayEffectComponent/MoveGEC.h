// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "SkillSystem/GameplayEffectComponent/BaseGECConfig.h"
#include "SkillSystem/SkillNiagaraSpawnSettings.h"
#include "MoveGEC.generated.h"

class USkillEffectDataAsset;
struct FGameplayEffectSpec;

// 이동 방식
UENUM(BlueprintType)
enum class EMoveType : uint8
{
	Teleport         UMETA(DisplayName = "Teleport (즉시 이동)"),
	Launch           UMETA(DisplayName = "Launch (Character::Launch)"),
	ConstantForce    UMETA(DisplayName = "ConstantForce (CMC::RootMotionSource)"),
};

// 이동 방향 결정 방식
UENUM(BlueprintType)
enum class EMoveDirectionSource : uint8
{
	Forward       UMETA(DisplayName = "캐릭터 전방"),
	TowardContext UMETA(DisplayName = "Context Origin 방향"),
	TowardTarget  UMETA(DisplayName = "Target Actor 방향"),
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UMoveGECConfig : public UBaseGECConfig
{
	GENERATED_BODY()

public:
	// --- 기본 이동 설정 ---
	UPROPERTY(EditDefaultsOnly, Category = "Move Settings|Base")
	EMoveType MoveType = EMoveType::Launch;

	UPROPERTY(EditDefaultsOnly, Category = "Move Settings|Base")
	EMoveDirectionSource DirectionSource = EMoveDirectionSource::Forward;

	UPROPERTY(EditDefaultsOnly, Category = "Move Settings|Base")
	float MoveDistance = 500.0f;

	// --- Launch / ConstantForce 공통 설정 ---
	UPROPERTY(EditDefaultsOnly, Category = "Move Settings|Movement",
		meta = (EditCondition = "MoveType != EMoveType::Teleport"))
	float MoveSpeed = 1500.0f;

	// --- Launch 전용 ---
	UPROPERTY(EditDefaultsOnly, Category = "Move Settings|Movement",
		meta = (EditCondition = "MoveType == EMoveType::Launch"))
	bool bXYOverride = true;

	UPROPERTY(EditDefaultsOnly, Category = "Move Settings|Movement",
		meta = (EditCondition = "MoveType == EMoveType::Launch"))
	bool bZOverride = false;

	// --- 충돌/안전 ---
	UPROPERTY(EditDefaultsOnly, Category = "Move Settings|Safety")
	bool bSweep = true; // 이동 중 충돌 체크 (Teleport 전용)

	UPROPERTY(EditDefaultsOnly, Category = "Move Settings|Safety")
	bool bIgnoreIfRootMotion = true; // 루트모션 애니메이션 재생 중이면 이동 무시

	UPROPERTY(EditDefaultsOnly, Category = "Move Settings|Safety")
	bool bIgnoreUnitCollision = false; // 이동 중 유닛(Pawn)과의 물리 충돌 무시

	// 지면 추적 거리 고정 (에디터 수정 불가)
	UPROPERTY(VisibleDefaultsOnly, Category = "Move Settings|Safety")
	float GroundTraceDistance = 500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Move Settings|Safety")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	// --- Wall Hit (벽꿍) 설정 ---
	UPROPERTY(EditDefaultsOnly, Category = "Move Settings|WallHit")
	bool bDetectWallHit = false; // 벽 충돌 감지 여부

	UPROPERTY(EditDefaultsOnly, Category = "Move Settings|WallHit",
		meta = (EditCondition = "bDetectWallHit"))
	TArray<TObjectPtr<USkillEffectDataAsset>> WallHitApplied; // 벽 충돌 시 적용할 효과

	// --- VFX ---
	UPROPERTY(EditDefaultsOnly, Category = "Move Settings|VFX")
	FSkillNiagaraSpawnSettings StartVfx; // 이동 시작 시

	UPROPERTY(EditDefaultsOnly, Category = "Move Settings|VFX")
	FSkillNiagaraSpawnSettings MovingVfx; // 이동 중 캐릭터에 부착

	UPROPERTY(EditDefaultsOnly, Category = "Move Settings|VFX")
	FSkillNiagaraSpawnSettings EndVfx; // 이동 완료 시
};

UCLASS()
class PROJECTER_API UMoveGEC : public UBaseGEC
{
	GENERATED_BODY()

public:
	UMoveGEC();

	virtual TSubclassOf<UBaseGECConfig> GetRequiredConfigClass() const override;

protected:
	virtual void OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;

private:
	// 현재 루트 모션 재생 여부 확인
	bool IsRootMotionActive(const AActor* Actor) const;

	// 이동 방향 계산
	FVector CalculateMoveDirection(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const UMoveGECConfig* Config) const;

	// 목적지 계산 (Sweep 충돌 포함) - Teleport 전용
	FVector CalculateDestination(const AActor* Instigator, const FVector& Direction, const UMoveGECConfig* Config) const;

	// 즉시 이동
	void ExecuteTeleport(AActor* Instigator, const FVector& Destination, const UMoveGECConfig* Config) const;

	// 물리 기반 발사
	void ExecuteLaunch(AActor* Instigator, const FVector& Direction, const UMoveGECConfig* Config) const;

	// Code-Driven 부드러운 이동 (CMC RootMotionSource)
	void ExecuteConstantForce(AActor* Instigator, const FVector& Direction, const UMoveGECConfig* Config, const FGameplayEffectSpec& GESpec) const;

	// 벽 충돌 처리 (벽꿍 효과 적용)
	void HandleWallHit(AActor* Instigator, const FHitResult& Hit, const UMoveGECConfig* Config, const FGameplayEffectSpec& GESpec) const;

	// 지면 보정 (도착 위치를 지면에 스냅)
	void SnapToGround(FVector& InOutLocation, const UMoveGECConfig* Config, const AActor* Instigator) const;

	// VFX 실행
	void ExecuteMoveCues(const FGameplayEffectSpec& GESpec, AActor* Instigator, const FVector& StartLocation, const FVector& EndLocation, const UMoveGECConfig* Config) const;
};
