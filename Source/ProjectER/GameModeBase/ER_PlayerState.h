// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ER_PlayerState.generated.h"



UCLASS()
class PROJECTER_API AER_PlayerState : public APlayerState
{
	GENERATED_BODY()
	AER_PlayerState();

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	void SetPlayerStateName(FString InputName) { PlayerStateName = InputName; }

	UFUNCTION(BlueprintCallable)
	void SetbIsReady() { bIsReady = !bIsReady; }

	UFUNCTION(BlueprintCallable)
	FString GetPlayerStateName() { return PlayerStateName; }

	UFUNCTION(BlueprintCallable)
	bool GetIsReady() { return bIsReady; }


public:
	UPROPERTY(Replicated, BlueprintReadOnly)
	FString PlayerStateName;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsReady = false;
	
};
