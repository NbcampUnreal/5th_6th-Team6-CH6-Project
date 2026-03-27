// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StaticGlobalUtils.generated.h"

#define LOOT_INTERACT_DISTANCE 300.f
/**
 * 
 */
UCLASS()
class PROJECTER_API UStaticGlobalUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "GlobalUtils")
	static float GetDistanceToActorBounds2D(const AActor* TargetActor, const FVector& FromLocation);
	
	UFUNCTION(BlueprintCallable, Category = "GlobalUtils")
	static FVector GetApproachLocationForActor(UWorld* World, const AActor* TargetActor, const FVector& FromLocation);


	// static float LootInteractDistance = 300.f; // << Define이동

};
