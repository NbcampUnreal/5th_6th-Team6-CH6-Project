#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstance.h"
#include "LineOfSight/VisionData.h"
#include "MainVisionRTManager.generated.h"

class UVision_VisualComp;
class ULOSRequirementPoolSubsystem;

/*
 * Composites all in-range LOS stamps onto CameraLocalRT each frame.
 *
 * Pool integration:
 *   Drives slot acquire/release on ULOSRequirementPoolSubsystem based on
 *   RectOverlapsWorld bounds check each frame.
 *   Calls are routed through UVision_VisualComp::OnPoolSlotAcquired /
 *   OnPoolSlotReleased — the manager never touches sub-components directly.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UMainVisionRTManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UMainVisionRTManager();

protected:
	virtual void BeginPlay() override;

public:

	UFUNCTION(BlueprintCallable, Category="LineOfSight")
	void InitializeMainVisionRTComp();

	UFUNCTION(BlueprintCallable, Category="LineOfSight")
	void UpdateCameraLOS();

	UCanvasRenderTarget2D* GetCameraLOSTexture() const { return CameraLocalRT; }

	UFUNCTION(BlueprintCallable, Category="LineOfSight")
	UMaterialInstanceDynamic* GetLayeredMID() const { return LayeredLOSInterfaceMID; }

private:

	UFUNCTION()
	void DrawLOS_CPU(UCanvas* Canvas, int32 Width, int32 Height);

	void DrawLOSStamp(
		UCanvas* Canvas,
		const TArray<UVision_VisualComp*>& Providers,
		const FLinearColor& Color);

	void RenderLOS_GPU(
		FRDGBuilder& GraphBuilder,
		FRDGTextureRef LOSTexture);

	bool ConvertWorldToRT(
		const FVector& ProviderWorldLocation,
		const float& ProviderVisionRange,
		FVector2D& OutPixelPosition,
		float& OutTileSize) const;

	bool GetVisibleProviders(TArray<UVision_VisualComp*>& OutProviders) const;

	bool ShouldRunClientLogic() const;

	void ApplyFeatheredBlurToRT();

	/** Lazy-cached pool subsystem — resolved once on first UpdateCameraLOS call. */
	ULOSRequirementPoolSubsystem* GetPoolSubsystem() const;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	bool bUseCPU = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	bool bDrawTextureRange = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	float CameraVisionRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	UCanvasRenderTarget2D* CameraLocalRT = nullptr;

#pragma region Blur

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	UCanvasRenderTarget2D* FeatheredRT = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	UMaterialInterface* FeatherOutMaterial = nullptr;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* FeatherMID = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	UMaterialInterface* BlurMaterial = nullptr;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* BlurMID = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision|Blur")
	float BlurRadiusPercent = 0.03f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision|Blur")
	int BlurNumSamples = 32;

	FVector2D CanvasSize = FVector2D{512.f, 512.f};

#pragma endregion

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	UMaterialParameterCollection* PostProcessMPC = nullptr;

	UPROPERTY()
	UMaterialParameterCollectionInstance* MPCInstance = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MPC")
	FName MPCLocationParam = TEXT("VisionCenterLocation");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MPC")
	FName MPCVisibleRangeParam = TEXT("VisibleRange");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MPC")
	FName MPCNearSightRangeParam = TEXT("NearSightRange");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	UMaterialInterface* LayeredLOSInterfaceMaterial = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vision")
	UMaterialInstanceDynamic* LayeredLOSInterfaceMID = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	FName LayeredLOSTextureParam = TEXT("RenderTarget");

	UPROPERTY(EditAnywhere, Category="Vision")
	uint32 CameraViewChannelMask = 0xFFFFFFFF;

	static uint32 MakeChannelBitMask(const TArray<EVisionChannel>& ChannelEnums);

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UVision_VisualComp>> CachedValidProviders;

	UPROPERTY(Transient)
	mutable TObjectPtr<ULOSRequirementPoolSubsystem> CachedPoolSubsystem = nullptr;
};
