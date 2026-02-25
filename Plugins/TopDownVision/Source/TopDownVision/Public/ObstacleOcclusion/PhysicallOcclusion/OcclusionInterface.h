// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "OcclusionInterface.generated.h"


//ForwardDeclares
class UStaticMeshComponent;


// This class does not need to be modified.
UINTERFACE()
class UOcclusionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TOPDOWNVISION_API IOcclusionInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)//when the overlapping actor is in the range
	void OnOcclusionEnter(UPrimitiveComponent* SourceComponent);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)//when the actor leave the range
	void OnOcclusionExit(UPrimitiveComponent* SourceComponent);
	
};
