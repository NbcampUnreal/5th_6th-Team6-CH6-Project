// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VolumeVisibilityEvaluator2D.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UVolumeVisibilityEvaluator2D : public UActorComponent
{
    GENERATED_BODY()

public:
    UVolumeVisibilityEvaluator2D();
    
    /*/** Generates points within the 2D footprint of the owner #1#
    UFUNCTION(BlueprintCallable, Category = "Visibility")
    void Bake2DFootprint(float FootprintRadius, int32 RingCount = 3);

protected:
    /** Local space points representing the character's floor footprint #1#
    UPROPERTY(VisibleAnywhere, Category = "Visibility")
    TArray<FVector2D> LocalFootprintPoints;

    /** Threshold to consider the actor "hidden" (e.g. 0.1 means 90% in shadow) #1#
    UPROPERTY(EditAnywhere, Category = "Visibility")
    float VisibilityThreshold = 0.15f;*/
};