#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectileData.generated.h"

class UNiagaraSystem;
class UStaticMesh;

UCLASS(BlueprintType)
class PROJECTER_API UProjectileData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	// [Visual] 투사체 메쉬
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMesh> ProjectileMesh;

	// [Visual] 투사체 크기
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "0.01"))
	FVector Scale = FVector(1.0f);
	
	// [Visual] 투사체 이펙트 (나이아가라) (ex: 불덩이, 마법 꼬리)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UNiagaraSystem> FlyVFX;
	
	// [Visual] 피격 시 이펙트 (나이아가라)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UNiagaraSystem> ImpactVFX;

	// [Movement] 초기 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float InitialSpeed = 2000.0f;

	// [Movement] 최대 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float MaxSpeed = 2000.0f;

	// [Movement] 중력 (0=직선, 1=곡사)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float GravityScale = 0.0f;

	// [게임플레이] 추가 데미지 계수 (예: 폭발 화살은 1.5배)
	// * 기본 데미지는 GAS Spec으로 넘어오지만, 투사체 자체의 특성을 반영할 때 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay", meta = (ClampMin = "0.0"))
	float DamageCoefficient = 1.0f;
};
