#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "CharacterSystem/Interface/TargetableInterface.h"
#include "BaseMonster.generated.h"

class UGameplayAbility;
class UStateTreeComponent;
class USphereComponent;
class UMonsterRangeComponent;
class UWidgetComponent;
class UUserWidget;
class UBaseMonsterAttributeSet;
class UGameplayEffect;
class ABaseCharacter;
class UMonsterDataAsset;
struct FOnAttributeChangeData;

UCLASS()
class PROJECTER_API ABaseMonster : public ACharacter, public IAbilitySystemInterface, public ITargetableInterface
{
	GENERATED_BODY()

public:

	ABaseMonster();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UStateTreeComponent* GetStateTreeComponent();

	void SetTargetPlayer(AActor* Target);
	AActor* GetTargetPlayer();

	void SetbIsCombat(bool Target);
	bool GetbIsCombat();

	void SetbIsDead(bool Target);
	bool GetbIsDead();

	void InitMonsterData(FPrimaryAssetId MonsterAssetId, float Level);

protected:

	virtual void PossessedBy(AController* newController) override;

	virtual void BeginPlay() override;
	
	virtual void Tick(float DeltaTime) override;



private:

	UFUNCTION()
	void OnRep_IsCombat();

	UFUNCTION()
	void OnRep_IsDead();

	UFUNCTION()
	void OnHealthChangedHandle(float CurrentHP, float MaxHP);

	UFUNCTION()
	void OnMoveSpeedChangedHandle(float OldSpeed, float NewSpeed);

	UFUNCTION(NetMulticast, BlueprintCallable, Reliable)
	void Multicast_SetDeathCollision();

	void GiveRewardsToPlayer(AActor* Player);

#pragma region Init

	void OnMonsterDataLoaded(FPrimaryAssetId LoadedId, float Level);

	void InitGiveAbilities();

	void InitAttributes(float Level);

	void InitVisuals();

	void InitHPBar();

#pragma endregion

#pragma region StateTree

	UFUNCTION()
	void OnMonterHitHandle(AActor* Target);

	UFUNCTION()
	void OnMonterDeathHandle(AActor* Target);
	
	UFUNCTION()
	void OnPlayerCountOneHandle();

	UFUNCTION()
	void OnPlayerCountZeroHandle();

	UFUNCTION(BlueprintCallable)
	void SendAttackRangeEvent(float AttackRange);

	UFUNCTION(BlueprintCallable)
	void SendReturnSuccessEvent();

#pragma endregion

public:

	// 몬스터 데이터 로드해서 참조
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "MonsterData")
	TObjectPtr<UMonsterDataAsset> MonsterData;

	// 사망 시 들고있을 박스 컴포넌트 변수
	// ABaseBoxComponent BoxComp;

protected:


private:

#pragma region GAS

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> ASC;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBaseMonsterAttributeSet> AttributeSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> XPRewardEffect;
    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Status", meta = (AllowPrivateAccess = "true"))
	FGameplayTag IncomingXPTag;

#pragma endregion

#pragma region Ability Tag

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Ability", meta = (AllowPrivateAccess = "true"))
	FGameplayTag DeathAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Ability", meta = (AllowPrivateAccess = "true"))
	FGameplayTag ChaseAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Ability", meta = (AllowPrivateAccess = "true"))
	FGameplayTag AttackAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Ability", meta = (AllowPrivateAccess = "true"))
	FGameplayTag ReturnAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Ability", meta = (AllowPrivateAccess = "true"))
	FGameplayTag SitAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Ability", meta = (AllowPrivateAccess = "true"))
	FGameplayTag IdleAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Ability", meta = (AllowPrivateAccess = "true"))
	FGameplayTag CombatAbilityTag;
#pragma endregion

#pragma region Event Tag

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Event", meta = (AllowPrivateAccess = "true"))
	FGameplayTag AttackEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Event", meta = (AllowPrivateAccess = "true"))
	FGameplayTag HitEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Event", meta = (AllowPrivateAccess = "true"))
	FGameplayTag DeathEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Event", meta = (AllowPrivateAccess = "true"))
	FGameplayTag BeginSearchEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Event", meta = (AllowPrivateAccess = "true"))
	FGameplayTag EndSearchEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Event", meta = (AllowPrivateAccess = "true"))
	FGameplayTag TargetOnEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Event", meta = (AllowPrivateAccess = "true"))
	FGameplayTag TargetOffEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Event", meta = (AllowPrivateAccess = "true"))
	FGameplayTag ReturnEventTag;

#pragma endregion

#pragma region State Tag

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|State", meta = (AllowPrivateAccess = "true"))
	FGameplayTag AliveStateTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|State", meta = (AllowPrivateAccess = "true"))
	FGameplayTag DeathStateTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|State", meta = (AllowPrivateAccess = "true"))
	FGameplayTag SitStateTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|State", meta = (AllowPrivateAccess = "true"))
	FGameplayTag IdleStateTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|State", meta = (AllowPrivateAccess = "true"))
	FGameplayTag CombatStateTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|State", meta = (AllowPrivateAccess = "true"))
	FGameplayTag MoveStateTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|State", meta = (AllowPrivateAccess = "true"))
	FGameplayTag AttackStateTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|State", meta = (AllowPrivateAccess = "true"))
	FGameplayTag ReturnStateTag;

#pragma endregion 

#pragma region StateTree

	// 서버만
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStateTreeComponent> StateTreeComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMonsterRangeComponent> MonsterRangeComp;

	// 서버에서 복제
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	FVector StartLocation;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	FRotator StartRotator;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadWrite, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> TargetPlayer;

	UPROPERTY(ReplicatedUsing = OnRep_IsCombat, VisibleAnywhere, BlueprintReadWrite, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	bool bIsCombat;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead, VisibleAnywhere, BlueprintReadWrite, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	bool bIsDead;

#pragma endregion

#pragma region UI

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowprivateAccess = "true"))
	TObjectPtr<UWidgetComponent> HPBarWidgetComp;

#pragma endregion

#pragma region TargetableInterface
public:
	// 팀 정보 반환
	virtual ETeamType GetTeamType() const override;

	// 타겟팅 가능 여부 반환
	virtual bool IsTargetable() const override;
    
	// [인터페이스 구현] 하이라이트 (나중에 포스트 프로세스로 구현)
	// virtual void HighlightActor(bool bIsHighlight) override;
	
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_SetTeamID(ETeamType NewTeamID);
	
protected:
	UFUNCTION()
	void OnRep_TeamID();
	
protected:
	// 팀 변수
	UPROPERTY(ReplicatedUsing = OnRep_TeamID, EditAnywhere, BlueprintReadWrite, Category = "Team")
	ETeamType TeamID;
	
#pragma endregion
	
	/// [전민성 추가분]

private:
	int32 SpawnPoint = 0;

public:
	// 임시로 사용할 사망 함수, 이후 맞는 위치로 이동 예정 
	void Death();

	void SetSpawnPoint(int32 point) { SpawnPoint = point; }
	
	int32 GetSpawnPoint() { return SpawnPoint; }



};



