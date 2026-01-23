// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ER_OutGamePlayerController.generated.h"


UCLASS()
class PROJECTER_API AER_OutGamePlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Network")
	void ConnectToDedicatedServer(const FString& Ip, int32 Port, const FString& UserName);

private:
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_StartGame();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_TEMP_OutGame();

};
