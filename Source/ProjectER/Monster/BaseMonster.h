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
class UBoxComponent;
class UMonsterRangeComponent;
class UWidgetComponent;
class UUserWidget;
class UBaseMonsterAttributeSet;
class UGameplayEffect;
class ABaseCharacter;
class UMonsterDataAsset;
class ULootableComponent;
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
	void OnRep_MonsterId();

	
	// 이벤트 태그
	UFUNCTION() // SendHitEvent()
	void OnMonterHitHandle(AActor* Target);

	UFUNCTION() // SendDeathEvent()
	void OnMonterDeathHandle(AActor* Target);

	UFUNCTION() // SendBeginSearchEvent();
	void OnPlayerCountOneHandle();

	UFUNCTION() // SendEndSearchEvent()
	void OnPlayerCountZeroHandle();

	UFUNCTION() // SendTargetOffEvent()
	void OnTargetLostHandle();

	UFUNCTION(BlueprintCallable) // SendAttackRangeEvent();
	void SendAttackRangeEvent(float AttackRange);

	UFUNCTION(BlueprintCallable) // SendReturnSuccessEvent();
	void SendReturnSuccessEvent();
	//

	// HealthBar 변경용
	UFUNCTION()
	void OnHealthChangedHandle(float CurrentHP, float MaxHP);
	// 이동 속도 변경값 적용
	UFUNCTION()
	void OnMoveSpeedChangedHandle(float OldSpeed, float NewSpeed);
	// 몬스터 사망 후 충돌을 꺼주는 함수
	

public:
	// 몬스터 스폰 후 데이터를 초기화해주는 함수
	void InitMonsterData(FPrimaryAssetId MonsterAssetId, float Level);
private:
	// 초기화 
	void InitMonsterDataLoading(FPrimaryAssetId MonsterAssetId, float Level);

	void OnMonsterDataLoaded(FPrimaryAssetId LoadedId, float Level);

	void InitGiveAbilities();

	void InitAttributes(float Level);

	void InitVisuals();

	void InitHPBar();
	//

	// 쿨다운 태그 관련
	UFUNCTION(BlueprintCallable)
	void OnCooldown(FGameplayTag CooldownTag, float Cooldown);

	void AddCooldownTag(FGameplayTag CooldownTag);

	void RemoveCooldownTag(FGameplayTag CooldownTag);

	//void CooldownCheck();
	//

	UFUNCTION(BlueprintCallable)
	bool HasASCTag(FGameplayTag Tag);

	UFUNCTION(NetMulticast, BlueprintCallable, Reliable)
	void Multicast_SetDeathCollision();

	UFUNCTION(BlueprintCallable)
	void GameplayEffectSetByCaller(AActor* Player, TSubclassOf<UGameplayEffect> GE, FGameplayTag Tag, float Amount);

	UFUNCTION(BlueprintCallable)
	void TryActivateByDynamicTag(FGameplayTag InputTag);

	UFUNCTION(BlueprintCallable)
	float GetAbilityDelayByTag(FGameplayTag InputTag);

	UFUNCTION(BlueprintCallable)
	float GetAbilityCoolTimeByTag(FGameplayTag InputTag);


public:
	// 블루프린트에서 사용중
	UPROPERTY(/*Replicated, */BlueprintReadOnly, Category = "MonsterData")
	TObjectPtr<UMonsterDataAsset> MonsterData;
private:
	// 해당값이 복제되면 클라이언트에서 데이터에셋 로드
	UPROPERTY(ReplicatedUsing = OnRep_MonsterId)
	FPrimaryAssetId MonsterId;

	float MonsterLevel;



public:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<ULootableComponent> LootableComp;
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStateTreeComponent> StateTreeComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMonsterRangeComponent> MonsterRangeComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> ASC;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBaseMonsterAttributeSet> AttributeSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowprivateAccess = "true"))
	TObjectPtr<UWidgetComponent> HPBarWidgetComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowprivateAccess = "true"))
	TObjectPtr<UBoxComponent> HitBoxComp;


#pragma region SetByCall Tag

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Status", meta = (AllowPrivateAccess = "true"))
	FGameplayTag IncomingXPTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> XPRewardEffect;

#pragma endregion

#pragma region Ability Tag

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Ability", meta = (AllowPrivateAccess = "true"))
	FGameplayTag DeathAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Ability", meta = (AllowPrivateAccess = "true"))
	FGameplayTag ChaseAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Ability", meta = (AllowPrivateAccess = "true"))
	FGameplayTag ReturnAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Ability", meta = (AllowPrivateAccess = "true"))
	FGameplayTag SitAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Ability", meta = (AllowPrivateAccess = "true"))
	FGameplayTag IdleAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Ability", meta = (AllowPrivateAccess = "true"))
	FGameplayTag CombatAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Ability", meta = (AllowPrivateAccess = "true"))
	FGameplayTag AttackAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Ability", meta = (AllowPrivateAccess = "true"))
	FGameplayTag QSkillAbilityTag;

#pragma endregion

#pragma region Cooldown Tag

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Cooldown", meta = (AllowPrivateAccess = "true"))
	FGameplayTag AutoAttackCooldownTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag|Cooldown", meta = (AllowPrivateAccess = "true"))
	FGameplayTag QSkillCooldownTag;

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

	// 서버에서 복제
	UPROPERTY(BlueprintReadOnly, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	FVector StartLocation;

	UPROPERTY(BlueprintReadOnly, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	FRotator StartRotator;

	UPROPERTY(BlueprintReadWrite, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> TargetPlayer;

	UPROPERTY(ReplicatedUsing = OnRep_IsCombat, VisibleAnywhere, BlueprintReadWrite, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	bool bIsCombat;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead, VisibleAnywhere, BlueprintReadWrite, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	bool bIsDead;

	//UPROPERTY(BlueprintReadWrite, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	//bool bIsAttackOnCooldown = true;

	//UPROPERTY(BlueprintReadWrite, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	//bool bIsQSkillOnCooldown = true;

#pragma endregion

#pragma region Timer

	TMap<FGameplayTag, FTimerHandle> CooldownTimerMap;

#pragma endregion

#pragma region TargetableInterface
public:
	// 팀 정보 반환
	virtual ETeamType GetTeamType() const override;

	// 타겟팅 가능 여부 반환
	virtual bool IsTargetable() const override;
    
	// [인터페이스 구현] 하이라이트 (나중에 포스트 프로세스로 구현)
	// virtual void HighlightActor(bool bIsHighlight) override;
	
	UFUNCTION()
	void OnRep_TeamID();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_SetTeamID(ETeamType NewTeamID);

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



