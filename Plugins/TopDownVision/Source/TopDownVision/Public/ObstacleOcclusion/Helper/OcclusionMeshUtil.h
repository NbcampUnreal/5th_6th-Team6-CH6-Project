#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "OcclusionMeshUtil.generated.h"

class UMeshComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class USplineMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(OcclusionMeshHelper, Log, All);

UCLASS()
class TOPDOWNVISION_API UOcclusionMeshUtil : public UObject
{
	GENERATED_BODY()

public:

	static void DiscoverChildMeshes(
		USceneComponent* Root,
		FName NormalTag,
		FName OccludedTag,
		TArray<TSoftObjectPtr<UMeshComponent>>& OutNormalMeshes,
		TArray<TSoftObjectPtr<UMeshComponent>>& OutOccludedMeshes);

	static void CreateDynamicMaterials_Static(
		const TArray<TSoftObjectPtr<UMeshComponent>>& Meshes,
		TArray<UMaterialInstanceDynamic*>& OutMIDs);

	static void CreateDynamicMaterials_Skeletal(
		const TArray<TSoftObjectPtr<UMeshComponent>>& Meshes,
		TArray<UMaterialInstanceDynamic*>& OutMIDs);

	static void GenerateShadowProxyMeshes(
		AActor* Owner,
		const TArray<TSoftObjectPtr<UMeshComponent>>& SourceMeshes,
		UMaterialInterface* ShadowProxyMaterial,
		TArray<TObjectPtr<UStaticMeshComponent>>& OutStaticProxies,
		TArray<TObjectPtr<USkeletalMeshComponent>>& OutSkeletalProxies);
};