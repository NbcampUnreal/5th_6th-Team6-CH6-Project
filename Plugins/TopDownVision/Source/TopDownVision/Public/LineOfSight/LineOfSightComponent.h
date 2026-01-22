// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LineOfSightComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API ULineOfSightComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULineOfSightComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnComponentCreated() override;
	
public:

private:
	//Helper functions
	
	
protected://variables
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LineOfSight)
	float VisionRange=500.f;
	
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LineOfSight)
	// custom trace channel here for user to change
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LineOfSight)
	TArray<AActor*> IgnoringActors;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LineOfSight)
	TArray<UPrimitiveComponent*> IgnoringComponents;


private:
	
	
};
