#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "OcclusionMeshUtil.generated.h"



// FD
class UMeshComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class USplineMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UOcclusionBinderSubsystem;
struct FOcclusionMIDSlot;

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(OcclusionMeshHelper, Log, All);

UCLASS()
class TOPDOWNVISION_API UOcclusionMeshUtil : public UObject
{
	GENERATED_BODY()

public:



#pragma region TagSetting
	
	static FName GetNormalMeshTag();
	static FName GetOccludedMeshTag();
	static FName GetRTMaterialTag();
	static FName GetNoShadowProxyTag();
	static FName GetAlphaParameterName();
	static FName GetForceOccludeParameterName();
	static FName GetRTSwitchParameterName();
	static FName GetOcclusionLockParameterName();
	
#pragma endregion
	

	//===== Find Meshes from the comp =====//
	
	static void DiscoverChildMeshes(
		USceneComponent* Root,
		TArray<TSoftObjectPtr<UMeshComponent>>& OutNormalMeshes,
		TArray<TSoftObjectPtr<UMeshComponent>>& OutOccludedMeshes);

	
#pragma region MID
	
	//==== Wrapper for both (make or pool) =====//
	static void AcquireMaterials(
		const TArray<TSoftObjectPtr<UMeshComponent>>& Meshes,
		const TArray<FName>& RequiredParameters,
		TArray<FOcclusionMIDSlot>& OutSlots,
		UOcclusionBinderSubsystem* Pool = nullptr);

	//===== Creating MID ======//
	
	static void CreateDynamicMaterials_Static(
		const TArray<TSoftObjectPtr<UMeshComponent>>& Meshes,
		TArray<UMaterialInstanceDynamic*>& OutMIDs);


	static void CreateDynamicMaterials_Skeletal(
		const TArray<TSoftObjectPtr<UMeshComponent>>& Meshes,
		TArray<UMaterialInstanceDynamic*>& OutMIDs);


	//==== Pooling ====//
	
	static void CheckoutMaterials(
		const TArray<TSoftObjectPtr<UMeshComponent>>& Meshes,
		UOcclusionBinderSubsystem* Pool,
		TArray<FOcclusionMIDSlot>& OutSlots);

	static void ReturnMaterials(
		TArray<FOcclusionMIDSlot>& Slots,
		UOcclusionBinderSubsystem* Pool);

	
	//=== Filter gate for checking if the material have param ====//
	// Returns true if the material has the given scalar parameter
	static bool DoesMaterialHaveScalarParameters(
		const UMaterialInterface* Material,
		const TArray<FName>& ParameterNames);

#pragma endregion MID



	//==== Shadow Proxy Making ====//

	static void GenerateShadowProxyMeshes(
		AActor* Owner,
		const TArray<TSoftObjectPtr<UMeshComponent>>& SourceMeshes,
		UMaterialInterface* ShadowProxyMaterial,
		//out
		TArray<TObjectPtr<UStaticMeshComponent>>& OutStaticProxies,
		TArray<TObjectPtr<USkeletalMeshComponent>>& OutSkeletalProxies);

	//=== TraceChannel getters ====//
	
	static TEnumAsByte<ECollisionChannel> GetOcclusionTraceChannel();
	static TEnumAsByte<ECollisionChannel> GetMouseTraceChannel();
	static TEnumAsByte<ECollisionChannel> GetInteriorTraceChannel();
};