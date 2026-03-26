// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BaseGameplayEffect.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTER_API UBaseGameplayEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UBaseGameplayEffect();
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
protected:

private:

public:
	/*UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameplayEffect|GameplayModifier", meta = (FilterMetaTag = "HideFromModifiers"))
	FGameplayAttribute SourceAttribute;	*/

protected:

private:
	
};
