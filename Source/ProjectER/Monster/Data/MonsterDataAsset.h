#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "StateTree.h"
#include "MonsterDataAsset.generated.h"

class UGameplayAbility;
class UBaseItemData;
class USkillDataAsset;
struct FGameplayTag;

UENUM(BlueprintType)
enum class EMonsterMontageType : uint8
{
	Idle,
	Move,
	Attack,
	QSkill,
	Sit,
	Dead,
	
	FlyStart,
	FlyIdle,
	FlyEnd
};


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

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Stat", meta = (Categories = "Unit.AttackType"))
	FGameplayTag AttackType;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Stat")
	TObjectPtr<UDataTable> MonsterDataTable;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Stat")
	TObjectPtr<UCurveTable> MonsterCurveTable;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Stat")
	TObjectPtr<UStateTree> MonsterStateTree;;


	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|GAS")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	// 스킬 시스템이 완성되면 사용
	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|GAS")
	TArray<TObjectPtr<USkillDataAsset>> SkillDataAssets;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterData|Montage")
	TMap<EMonsterMontageType, TObjectPtr<UAnimMontage>> Montages;



	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Visual")
	TObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Visual")
	FVector MeshScale;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Visual")
	float CollisionRadius;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Visual")
	float CapsuleHalfHeight;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Visual")
	FVector HitBoxExtent;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Visual")
	TSubclassOf<UAnimInstance> Anim;



	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Reward")
	int Exp;

	// 안씀
	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Reward")
	int Gold;

	// 이거는 죽었을 때 로드해서 ??
	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Reward")
	TArray<UBaseItemData*> ItemList;

};
