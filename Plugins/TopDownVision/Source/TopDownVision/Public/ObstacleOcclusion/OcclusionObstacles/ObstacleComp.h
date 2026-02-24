// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ObstacleComp.generated.h"

/**
 * This is for the Stylized obstacle, which need to show the occluded mesh when it need to show the target behind 
 * 
 */


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UObstacleComp : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UObstacleComp();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;




protected:
	//Mesh
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* NormMesh;// this is for the non-occluded mesh

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* OccludedMesh;
	
};
