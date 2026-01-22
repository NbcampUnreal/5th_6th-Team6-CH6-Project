#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
//#include "Abilities/GameplayAbility.h"
#include "BaseMonster.generated.h"

class UBaseMonsterAttributeSet;
class UGameplayAbility;
class UStateTreeComponent;

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
	void InitAbilitySystem();

	void InitGiveAbilities();

public:


protected:


private:

#pragma region GAS
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> ASC;

	UPROPERTY()
	TObjectPtr<UBaseMonsterAttributeSet> AttributeSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;


#pragma endregion

};



