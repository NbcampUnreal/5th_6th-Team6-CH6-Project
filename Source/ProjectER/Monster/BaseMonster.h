#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "BaseMonster.generated.h"

class UBaseMonsterAttributeSet;
class UGameplayAbility;
class UStateTreeComponent;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttributeChangeDelegate, float, CurrentValue, float, MaxValue);

UCLASS()
class PROJECTER_API ABaseMonster : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABaseMonster();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;


protected:
	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* newController) override;


private:

	UFUNCTION(BlueprintCallable, meta = (AllowprivateAccess = "ture"))
	virtual void TryActivateAttackAbility();

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
	FGameplayTag AttackAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag", meta = (AllowPrivateAccess = "true"))
	FGameplayTag CombatTag;

#pragma endregion

#pragma region StateTree

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<APawn> TargetPlayer;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FVector TargetPlayerLocation;

#pragma endregion
};



