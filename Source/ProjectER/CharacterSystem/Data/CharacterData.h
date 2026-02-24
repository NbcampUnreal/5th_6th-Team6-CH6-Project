#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CharacterData.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class USkeletalMesh;
class UAnimInstance;
class USkillDataAsset;
class ABaseProjectile;

UCLASS(BlueprintType, Const)
class PROJECTER_API UCharacterData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	// 캐릭터 Mesh
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<USkeletalMesh> Mesh;
	
	// 캐릭터 Animation Blueprint
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	TSoftClassPtr<UAnimInstance> AnimClass;
	
	// 사망 몽타주 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<UAnimMontage> DeathMontage;
	
	// 기본 스킬 (일반 공격, 아군 살리기 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
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
};
