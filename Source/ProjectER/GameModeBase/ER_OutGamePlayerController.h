#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ER_OutGamePlayerController.generated.h"


UCLASS()
class PROJECTER_API AER_OutGamePlayerController : public APlayerController
{
	GENERATED_BODY()
	
private:
	virtual void BeginPlay() override;


public:
	UFUNCTION(BlueprintCallable, Category = "Network")
	void ConnectToDedicatedServer(const FString& Ip, int32 Port, const FString& PlayerName);

private:
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_StartGame();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_TEMP_OutGame();


	// 인 게임 컨트롤러로 이식 필요
	void HandleLoseChanged(bool bLose);
	void HandleWinChanged(bool bWin);


	// UI 출력 테스트
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> LoseUIClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> LoseUIInstance;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> WinUIClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> WinUIInstance;
};
