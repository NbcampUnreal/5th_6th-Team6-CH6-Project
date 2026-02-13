// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ER_NeutralSpawnSubsystem.generated.h"

class AActor;
class ACharacter;
class ABaseMonster;
struct FNeutralClassConfig;

USTRUCT(BlueprintType)
struct FNeutralInfo
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<AActor> SpawnPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ACharacter> NeutralActorClass;

	TWeakObjectPtr<ABaseMonster> SpawnedActor;

	FTimerHandle RespawnTimer;

	FName DAName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RespawnDelay = 5.f;

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsSpawned = false;
};

UCLASS()
class PROJECTER_API UER_NeutralSpawnSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void InitializeSpawnPoints(TMap<FName, FNeutralClassConfig>& NeutralClass);
	void StartRespawnNeutral(const int32 SpawnPointIdx);
	void FirstSpawnNeutral();
	void SetFalsebIsSpawned(const int32 SpawnPointIdx);

	void RegisterPoint(AActor* Point);
	void UnregisterPoint(AActor* Point);

	void TEMP_SpawnNeutrals();
	void TEMP_NeutralsALLDespawn();


public:
	TArray<TWeakObjectPtr<AActor>> Points;

private:

	UPROPERTY()
	TMap<int32, FNeutralInfo> NeutralSpawnMap;

	UPROPERTY()
	bool bIsInitialized = false;
};
