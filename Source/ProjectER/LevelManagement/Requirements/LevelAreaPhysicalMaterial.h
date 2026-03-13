// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
//#include "LevelManagement/Requirements/LevelAreaData.h"// now it can also have the hazard state as well. simple setter can make it work
//--> nope, this will only work for the MID. just get id and in subsystem, compare to check the safety state
#include "LevelAreaPhysicalMaterial.generated.h"

/**
 *  this use material's physic material as parent so that mesh can have id using material
 *
 *  this will be used for area tracker trace value
 */

UCLASS()
class PROJECTER_API ULevelAreaPhysicalMaterial : public UPhysicalMaterial
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Area")
    int32 NodeID = INDEX_NONE;
};
