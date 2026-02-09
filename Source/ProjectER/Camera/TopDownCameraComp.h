// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TopDownCameraComp.generated.h"

// this will manage the main RT(following the camera arm root)
/**
 *
 *  also this will manage obstacle occlusion effect which will make the obstacle hiding the vision area 
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTER_API UTopDownCameraComp : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UTopDownCameraComp();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};
