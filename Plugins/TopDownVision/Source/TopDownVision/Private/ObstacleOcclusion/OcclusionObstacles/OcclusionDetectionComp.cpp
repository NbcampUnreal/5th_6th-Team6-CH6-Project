// Fill out your copyright notice in the Description page of Project Settings.


#include "TopDownVision/Public/ObstacleOcclusion/PhysicallOcclusion/OcclusionDetectionComp.h"


UOcclusionDetectionComp::UOcclusionDetectionComp()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UOcclusionDetectionComp::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner=GetOwner();
	if (!Owner)
	{
		//failed to get valid owner
		return;
	}

	for (int32 i=0;i<SphereComponents.Num();++i)//loop the sphere comps
	{
		
	}
	
}

void UOcclusionDetectionComp::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UOcclusionDetectionComp::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
}

void UOcclusionDetectionComp::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}


// Called every frame
void UOcclusionDetectionComp::TickComponent(float DeltaTime, ELevelTick TickType,
                                            FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

