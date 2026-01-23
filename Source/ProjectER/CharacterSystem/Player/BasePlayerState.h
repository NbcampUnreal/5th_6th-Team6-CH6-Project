#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "BasePlayerState.generated.h"

class UAbilitySystemComponent;
class UBaseAttributeSet;

UCLASS()
class PROJECTER_API ABasePlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	ABasePlayerState();
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	UBaseAttributeSet* GetAttributeSet() const { return AttributeSet; }
	
private:
	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UBaseAttributeSet> AttributeSet;
};
