// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "OcclusionMeshUtil.generated.h"

/**
 *  Helper for finding the mesh 
 */

//FD
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(OcclusionMeshHelper, Log, All);

UCLASS()
class TOPDOWNVISION_API UOcclusionMeshUtil : public UObject
{
	GENERATED_BODY()
	
public:
	
	// Discover meshes using tags
	static void DiscoverChildMeshes(
		USceneComponent* Root,
		FName NormalTag,
		FName OccludedTag,
		TArray<TSoftObjectPtr<UStaticMeshComponent>>& OutNormalMeshes,
		TArray<TSoftObjectPtr<UStaticMeshComponent>>& OutOccludedMeshes);

	// Create dynamic materials
	static void CreateDynamicMaterials(
		const TArray<TSoftObjectPtr<UStaticMeshComponent>>& Meshes,
		TArray<UMaterialInstanceDynamic*>& OutMIDs);
	
};
