#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "StateTree.h"
#include "MonsterDataAsset.generated.h"

class UGameplayAbility;
class UBaseItemData;
class USkillDataAsset;
class UNiagaraSystem;
struct FGameplayTag;

UENUM(BlueprintType)
enum class EMonsterMontageType : uint8
{
	Idle,
	Alert,
	Move,
	Attack,
	QSkill,
	WSkill,
	ESkill,
	RSkill,
	Dead,
	
	FlyStart,
	FlyAttack,
	FlyEnd,
	None
};

UENUM(BlueprintType)
enum class EMonsterActionType : uint8
{
	Idle,
	Alert,
	Move,
	Death,
	NormalAttack,
	QSkill,
	WSkill,
	ESkill,
	RSkill,
	FlyStart,
	FlyAttack,
	FlyEnd,
	None
};

UENUM(BlueprintType)
enum class ENiagaraAttachType : uint8
{
	Mine,
	Target,
	None
};

UENUM(BlueprintType)
enum class ENiagaraSpawnPositionType : uint8
{
	TargetPosition,
	TargetDirection,
	None
};

USTRUCT(BlueprintType)
struct FMonsterNiagaraData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UNiagaraSystem> NiagaraSystem;



	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FVector PositionOffset = FVector(0,0,0);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FRotator RotationOffset = FRotator(0,0,0);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bFollow = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bFollow", EditConditionHides))
	ENiagaraAttachType AttachType = ENiagaraAttachType::Mine;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bFollow", EditConditionHides))
	FName AttachSocket = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "!bFollow", EditConditionHides))
	ENiagaraSpawnPositionType SpawnType = ENiagaraSpawnPositionType::TargetPosition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "!bFollow", EditConditionHides))
	FVector Scale = FVector(1.f);

};

USTRUCT(BlueprintType)
struct FMonsterSoundData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<USoundBase> TestSound;
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterData|Effect")
	TMap<EMonsterActionType, FMonsterNiagaraData> Niagaras;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterData|Effect")
	TMap<EMonsterActionType, FMonsterSoundData> Sounds;



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
