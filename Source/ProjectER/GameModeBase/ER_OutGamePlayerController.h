#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ER_OutGamePlayerController.generated.h"


UCLASS()
class PROJECTER_API AER_OutGamePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AER_OutGamePlayerController();
	
private:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable, Category = "Network")
	void ConnectToDedicatedServer(const FString& Ip, int32 Port, const FString& PlayerName);

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_SetLose();

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_SetWin();

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_SetDead();

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_StartRespawnTimer();

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_StopRespawnTimer();

private:
	void ShowWinUI();

	void ShowLoseUI();

	void ShowRespawnTimerUI();

	void HideRespawnTimerUI();


	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_StartGame();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_DisConnectServer();


private:
	// UI 출력
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> LoseUIClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> LoseUIInstance;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> WinUIClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> WinUIInstance;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> RespawnUIClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> RespawnUIInstance;
};
