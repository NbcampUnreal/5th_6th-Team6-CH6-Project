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

UCLASS(BlueprintType, Const)
class PROJECTER_API UCharacterData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	// 캐릭터 Mesh
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TSoftObjectPtr<USkeletalMesh> Mesh;
	
	// 캐릭터 Animation Blueprint
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TSoftClassPtr<UAnimInstance> AnimClass;
	
	// 기본 스킬 (일반 공격 : Auto Attack)
	UPROPERTY(EditDefaultsOnly, Category = "AutoAttack")
	TMap<FGameplayTag, TSoftClassPtr<UGameplayAbility>> Abilities;
	
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
