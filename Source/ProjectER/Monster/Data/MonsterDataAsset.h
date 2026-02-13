#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "MonsterDataAsset.generated.h"

class UGameplayAbility;
class UBaseItemData;
struct FGameplayTag;

// 몬스터 데이터
UCLASS()
class PROJECTER_API UMonsterDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	
public:

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Stat")
	FName TableRowName; // 해당 몬스터 Row 이름

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Stat")
	TObjectPtr<UDataTable> MonsterDataTable;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Stat")
	TObjectPtr<UCurveTable> MonsterCurveTable;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|GAS")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterData|Montage")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterData|Montage")
	TObjectPtr<UAnimMontage> DeadMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterData|Montage")
	TObjectPtr<UAnimMontage> IdleMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterData|Montage")
	TObjectPtr<UAnimMontage> MoveMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterData|Montage")
	TObjectPtr<UAnimMontage> SitMontage;


	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Visual")
	TObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Visual")
	TSubclassOf<UAnimInstance> Anim;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Reward")
	int Exp;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Reward")
	int Gold;

	// 이거는 죽었을 때 로드해서 ??
	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Reward")
	TArray<UBaseItemData*> ItemList;

};
