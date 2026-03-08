#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "LineOfSight/ObjectTracing/TopDown2DShapeData.h"//for 2d shape
#include "CharacterData.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class USkeletalMesh;
class UAnimInstance;
class UNiagaraSystem;
class USkillDataAsset;
class ABaseProjectile;

UCLASS(BlueprintType, Const)
class PROJECTER_API UCharacterData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	//==== Mesh visuals ====//
	// 캐릭터 Mesh
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	//Detection Shape Information

	/*UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	E2DShapeType TopDown2DShapeType=E2DShapeType::None;*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Detection")
	TArray<FVector2D> DetectionSamplePoints;
	
	// 캐릭터 Animation Blueprint
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	TSoftClassPtr<UAnimInstance> AnimClass;
	
	// 캐릭터 Animation Montages
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	TMap<FGameplayTag, TSoftObjectPtr<UAnimMontage>> CharacterMontages;
	
	// 일반 공격 피격 이펙트 (나이아가라)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX|AutoAttack")
	TSoftObjectPtr<UNiagaraSystem> BasicHitVFX;
	
	// 피격 이펙트 크기 (Scale) 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX|AutoAttack")
	FVector BasicHitVFXScale = FVector(1.0f);
	
	// 부활 시 출력 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Revive")
	TSoftObjectPtr<class UNiagaraSystem> ReviveVFX;
    
	// 부활 이펙트 크기 (Scale) 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Revive")
	FVector ReviveVFXScale = FVector(1.0f);
	
	// 공통 스킬 (일반 공격, 아군 살리기 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Common")
	TMap<FGameplayTag, TSoftClassPtr<UGameplayAbility>> Abilities;
	
	// 원거리 발사체 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Ranged")
	TSubclassOf<ABaseProjectile> ProjectileClass;

	// 발사 소켓 이름 (예: "Muzzle_01", "ArrowSocket")
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Ranged")
	FName MuzzleSocketName;
	
	// 특수 스킬 (Q, W, E, R)
	UPROPERTY(EditDefaultsOnly, Category = "SkillDataAsset")
	TArray<TSoftObjectPtr<USkillDataAsset>> SkillDataAsset;
	
	// 스탯 커브 테이블 (Curve Table Pointer)
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	TSoftObjectPtr<UCurveTable> StatCurveTable;
	
	// 커브 테이블 행 이름   (Curve Table Row Name)
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	FName StatusRowName;

	// 
};
