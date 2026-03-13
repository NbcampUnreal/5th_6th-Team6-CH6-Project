#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "OcclusionMeshUtil.generated.h"

class UMeshComponent;
class UMaterialInstanceDynamic;

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(OcclusionMeshHelper, Log, All);

UCLASS()
class TOPDOWNVISION_API UOcclusionMeshUtil : public UObject
{
	GENERATED_BODY()

public:

	// Discovers both static and skeletal meshes using tags
	static void DiscoverChildMeshes(
		USceneComponent* Root,
		FName NormalTag,
		FName OccludedTag,
		TArray<TSoftObjectPtr<UMeshComponent>>& OutNormalMeshes,
		TArray<TSoftObjectPtr<UMeshComponent>>& OutOccludedMeshes);

	// Creates dynamic materials for both static and skeletal meshes
	static void CreateDynamicMaterials(
		const TArray<TSoftObjectPtr<UMeshComponent>>& Meshes,
		TArray<UMaterialInstanceDynamic*>& OutMIDs);
};