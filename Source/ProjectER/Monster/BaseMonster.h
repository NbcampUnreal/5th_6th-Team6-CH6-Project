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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttributeChangeDelegate, float, CurrentValue, float, MaxValue);

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
	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* newController) override;


private:

	void InitAbilitySystem();

	void InitGiveAbilities();

	void OnHealthChangedCallback(const FOnAttributeChangeData& Data) const;


public:

#pragma region Delegate

	UPROPERTY(BlueprintAssignable, Category = "AttributeDelegate")
	FOnAttributeChangeDelegate OnHealthChanged;

#pragma endregion


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

#pragma region Tag

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag DeathAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag MoveToActorAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag AttackAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag ReturnAbilityTag;

#pragma endregion

#pragma region State

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStateTreeComponent> StateTreeComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMonsterRangeComponent> MonsterRangeComp;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadWrite, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	AActor* TargetPlayer;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	FVector StartLocation;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadWrite, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	bool bIsCombat;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadWrite, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	bool bIsDead;

#pragma endregion
};



