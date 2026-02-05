// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AnimNotify_AddTagSkillActive.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTER_API UAnimNotify_AddTagSkillActive : public UAnimNotify
{
	GENERATED_BODY()
public:
	UAnimNotify_AddTagSkillActive();

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayAbility")
	float EventMagnitude = 1.0f;

private:
	UPROPERTY(VisibleAnywhere, Category = "Tag")
	FGameplayTag ActiveTag;
};
