#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "MonsterDataAsset.generated.h"

class UGameplayAbility;
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
	TSoftObjectPtr<UDataTable> MonsterDataTable;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Stat")
	TSoftObjectPtr<UCurveTable> MonsterCurveTable;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|GAS")
	TArray<TSoftClassPtr<UGameplayAbility>> DefaultAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Visual")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Visual")
	TSoftClassPtr<UAnimInstance> Anim;


};
