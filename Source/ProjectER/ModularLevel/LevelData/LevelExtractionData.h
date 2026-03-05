// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LevelExtractionData.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FRoomActorData
{
	GENERATED_BODY()

	// Use soft class reference (better for large systems)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftClassPtr<AActor> ActorClass;

	// Relative position to LevelRootActor
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector RelativeLocation = FVector::ZeroVector;
	// Relative rotation to LevelRootActor
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRotator RelativeRotation = FRotator::ZeroRotator;
};

UCLASS(BlueprintType)
class PROJECTER_API ULevelExtractionData : public UDataAsset
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString Description = TEXT("Description here.");
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Extraction")
	bool bIsLocked = false;// toggle modification access

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Extraction", meta=(EditCondition="!bIsLocked"))
	TArray<FRoomActorData> Actors;
};