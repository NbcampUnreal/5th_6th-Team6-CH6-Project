// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ER_PointActor.generated.h"

UENUM(BlueprintType)
enum class EPointActorType : uint8
{
	None,
	SpawnPoint,
	RespawnPoint,
	ObjectPoint,
};


UCLASS()
class PROJECTER_API AER_PointActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AER_PointActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(BlueprintReadWrite)
	EPointActorType PointType = EPointActorType::None;

};
