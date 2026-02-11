// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModeBase/PointActor/ER_PointActor.h"
#include "GameModeBase/Subsystem/Respawn/ER_RespawnSubsystem.h"

// Sets default values
AER_PointActor::AER_PointActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AER_PointActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void AER_PointActor::EndPlay(const EEndPlayReason::Type Reason)
{

	Super::EndPlay(Reason);
}

// Called every frame
void AER_PointActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

