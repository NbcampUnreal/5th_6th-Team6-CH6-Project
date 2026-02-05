// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_SendServerEvent.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTER_API UAbilityTask_SendServerEvent : public UAbilityTask
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (DisplayName = "SendServerEvent", HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
    static UAbilityTask_SendServerEvent* SendServerEvent(UGameplayAbility* OwningAbility, FGameplayTag Tag);

    virtual void Activate() override;

    UFUNCTION(Server, Reliable)
    void Server_SendEvent(FGameplayTag Tag);

    FGameplayTag EventTag;
};
