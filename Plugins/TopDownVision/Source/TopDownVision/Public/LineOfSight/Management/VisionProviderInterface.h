// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

//#include "CoreMinimal.h" -> no need for pure interface
#include "UObject/Interface.h"
#include "VisionProviderInterface.generated.h"

/*
 * To Interact without casting, use TScriptInterface to pair and store them
 * 
 */
UINTERFACE()
class UVisionProviderInterface : public UInterface
{
	GENERATED_BODY()
};


class TOPDOWNVISION_API IVisionProviderInterface
{
	GENERATED_BODY()

public:
	virtual uint8 GetVisionTeam() const = 0;// no ufunction, but just pure virtual function
	virtual FVector GetVisionOrigin() const = 0;
	virtual float GetVisionRadius() const = 0;

	virtual void GetVisibleActors(TArray<AActor*>& OutActors) const = 0;//get the actors visible to the local
};
