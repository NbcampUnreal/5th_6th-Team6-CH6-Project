// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TEMPNeutral.generated.h"

UCLASS()
class PROJECTER_API ATEMPNeutral : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ATEMPNeutral();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void Death();

	void Respawn();

	int32 Spawnpoint;

	void SetSpawnpoint(int32 point){ Spawnpoint = point; }
	int32 GetSpawnPoint() { return Spawnpoint; }
};
