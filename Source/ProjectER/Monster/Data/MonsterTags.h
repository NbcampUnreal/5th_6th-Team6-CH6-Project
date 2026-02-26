// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MonsterTags.generated.h"

USTRUCT(BlueprintType)
struct PROJECTER_API FMonsterTags
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag IncomingXPTag;


	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag AttackAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag QSkillAbilityTag;



	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag AttackEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag HitEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag DeathEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag BeginSearchEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag EndSearchEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag TargetOnEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag TargetOffEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag ReturnEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag Phase1EventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag Phase2EventTag;


	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag DeathStateTag;


};
