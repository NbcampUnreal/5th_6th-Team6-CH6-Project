#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameModeBase/PointActor/ER_PointActor.h"
#include "ER_ObjectSubsystem.generated.h"

struct FObjectClassConfig;

USTRUCT(BlueprintType)
struct FObjectInfo
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<AActor> SpawnPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> ObjectClass;

	//TWeakObjectPtr<ABaseMonster> SpawnedActor;

	FName DAName;

	bool bIsSpawned = false;

	ERegionType RegionType;
};

UCLASS()
class PROJECTER_API UER_ObjectSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	void InitializeObjectPoints(TMap<FName, FObjectClassConfig>& ObjectClass);

	void SpawnSupplyOjbect();

	void RegisterPoint(AActor* Point);
	void UnregisterPoint(AActor* Point);

public:
	TArray<TWeakObjectPtr<AActor>> Points;



private:
	UPROPERTY()
	bool bIsInitialized = false;

	// 항공 보급 위치를 모아둘 배열
	TMap<ERegionType, TArray<FObjectInfo>> SupplyPointsByRegion;

	// 보스 스폰 위치를 모아둘 배열
	TArray<FObjectInfo> BossPoints;

	// 이후 생, 운 포인트가 나오면 배열 추가하기
};
