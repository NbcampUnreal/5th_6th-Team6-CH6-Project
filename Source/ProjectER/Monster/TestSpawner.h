#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestSpawner.generated.h"

UCLASS()
class PROJECTER_API ATestSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	ATestSpawner();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, Category = "Test")
	TSubclassOf<AActor> Monster;

	UPROPERTY(EditAnywhere, Category = "Test")
	int32 SpawnCount;
};
