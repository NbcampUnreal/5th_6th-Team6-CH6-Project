// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "ER_SessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionCreateComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionFindComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionJoinComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionDestroyComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionStartComplete, bool, bWasSuccessful);

/**
 * Wrapper struct to expose Session Search Results to Blueprints
 */
USTRUCT(BlueprintType)
struct PROJECTER_API FSessionResultWrapper
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "ER|Session")
	int32 SessionIndex = -1;

	UPROPERTY(BlueprintReadOnly, Category = "ER|Session")
	int32 Ping = 0;

	UPROPERTY(BlueprintReadOnly, Category = "ER|Session")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "ER|Session")
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "ER|Session")
	FString OwningUserName;
};

/**
 * Subsystem for managing Online Sessions (Steam, LAN)
 */
UCLASS()
class PROJECTER_API UER_SessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UER_SessionSubsystem();

	//~ Begin UGameInstanceSubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End UGameInstanceSubsystem Interface

	/** Creates a new online session */
	UFUNCTION(BlueprintCallable, Category = "ER|Session")
	void CreateSession(int32 NumPublicConnections, bool bIsLANMatch);

	/** Finds available online sessions */
	UFUNCTION(BlueprintCallable, Category = "ER|Session")
	void FindSessions(int32 MaxSearchResults, bool bIsLANMatch);

	/** Joins a session using an index from the found search results */
	UFUNCTION(BlueprintCallable, Category = "ER|Session")
	void JoinSession(int32 SessionIndex);

	/** Destroys the current online session */
	UFUNCTION(BlueprintCallable, Category = "ER|Session")
	void DestroyGameSession();

	/** Starts the current online session */
	UFUNCTION(BlueprintCallable, Category = "ER|Session")
	void StartGameSession();

	/** Gets the cached search results from the last FindSessions call */
	UFUNCTION(BlueprintCallable, Category = "ER|Session")
	TArray<FSessionResultWrapper> GetCustomSearchResults() const;


public:
	// Delegates for UI or to notify other systems
	UPROPERTY(BlueprintAssignable, Category = "ER|Session|Delegates")
	FOnSessionCreateComplete OnCreateSessionCompleteEvent;

	UPROPERTY(BlueprintAssignable, Category = "ER|Session|Delegates")
	FOnSessionFindComplete OnFindSessionsCompleteEvent;

	UPROPERTY(BlueprintAssignable, Category = "ER|Session|Delegates")
	FOnSessionJoinComplete OnJoinSessionCompleteEvent;

	UPROPERTY(BlueprintAssignable, Category = "ER|Session|Delegates")
	FOnSessionDestroyComplete OnDestroySessionCompleteEvent;

	UPROPERTY(BlueprintAssignable, Category = "ER|Session|Delegates")
	FOnSessionStartComplete OnStartSessionCompleteEvent;

protected:
	// Internal callbacks bound to the IOnlineSession interface
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);

private:
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<class FOnlineSessionSearch> LastSessionSearch;

};
