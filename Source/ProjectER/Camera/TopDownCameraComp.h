// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TopDownCameraComp.generated.h"

/**
 *  this is for camera comp. the location will be used for RT and also curved World Origin Param value
 */

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTER_API UTopDownCameraComp : public USceneComponent
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
