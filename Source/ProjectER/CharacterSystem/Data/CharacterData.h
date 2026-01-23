#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CharacterData.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class USkeletalMesh;
class UAnimInstance;

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
	
	// GAS 스킬 (Q, W, E, R, Passive)
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TMap<FGameplayTag, TSoftClassPtr<UGameplayAbility>> Abilities;
	
	// 스탯 커브 테이블 (Curve Table Pointer)
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	TSoftObjectPtr<UCurveTable> StatCurveTable;
	
	// 커브 테이블 행 이름 (Curve Table Row Name)
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	FName StatusRowName;
};
