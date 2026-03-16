#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "OcclusionMeshUtil.generated.h"

class UMeshComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;

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

	// Creates MIDs for static meshes only — all material slots per mesh
	static void CreateDynamicMaterials_Static(
		const TArray<TSoftObjectPtr<UMeshComponent>>& Meshes,
		TArray<UMaterialInstanceDynamic*>& OutMIDs);

	// Creates MIDs for skeletal meshes only — all material slots per mesh
	static void CreateDynamicMaterials_Skeletal(
		const TArray<TSoftObjectPtr<UMeshComponent>>& Meshes,
		TArray<UMaterialInstanceDynamic*>& OutMIDs);

	// Generates hidden shadow proxy meshes for a given source mesh array.
	// Proxies are fully opaque (ShadowProxyMaterial), hidden in game, cast shadows only.
	// Existing proxies are destroyed and replaced on each call.
	static void GenerateShadowProxyMeshes(
		AActor* Owner,
		const TArray<TSoftObjectPtr<UMeshComponent>>& SourceMeshes,
		UMaterialInterface* ShadowProxyMaterial,
		TArray<TObjectPtr<UStaticMeshComponent>>& OutStaticProxies,
		TArray<TObjectPtr<USkeletalMeshComponent>>& OutSkeletalProxies);
};