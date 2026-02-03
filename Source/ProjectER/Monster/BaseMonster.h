#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "BaseMonster.generated.h"

class UBaseMonsterAttributeSet;
class UGameplayAbility;
class UStateTreeComponent;
class USphereComponent;
class UMonsterRangeComponent;
class UWidgetComponent;
class ABaseCharacter;
struct FOnAttributeChangeData;

UCLASS()
class PROJECTER_API ABaseMonster : public ACharacter, public IAbilitySystemInterface
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

	void InitGiveAbilities();

	UFUNCTION()
	void OnHealthChangedHandle(float OldValue, float NewValue);

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

public:


protected:


private:

#pragma region GAS

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> ASC;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBaseMonsterAttributeSet> AttributeSet;
    
#pragma endregion

#pragma region Ability Tag

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag DeathAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag ChaseAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag AttackAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag ReturnAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag SitAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag IdleAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag CombatAbilityTag;
#pragma endregion

#pragma region Event Tag

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag AttackEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag HitEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag DeathEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag BeginSearchEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag EndSearchEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag TargetOnEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag TargetOffEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag ReturnEventTag;

#pragma endregion

#pragma region State Tag

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag AliveStateTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag DeathStateTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag SitStateTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag IdleStateTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag CombatStateTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag MoveStateTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag AttackStateTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag ReturnStateTag;

#pragma endregion 

#pragma region State

	// 서버만
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStateTreeComponent> StateTreeComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMonsterRangeComponent> MonsterRangeComp;

	// 서버에서 복제
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	FVector StartLocation;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadWrite, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	AActor* TargetPlayer;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadWrite, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	bool bIsCombat;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadWrite, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	bool bIsDead;


#pragma endregion
};



