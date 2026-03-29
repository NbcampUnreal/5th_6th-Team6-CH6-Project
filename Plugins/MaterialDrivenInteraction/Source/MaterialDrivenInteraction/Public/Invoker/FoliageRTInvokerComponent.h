#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Data/RTBrushTypes.h"
#include "FoliageRTInvokerComponent.generated.h"

class URTPoolManager;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS(ClassGroup = "Foliage RT", meta = (BlueprintSpawnableComponent))
class MATERIALDRIVENINTERACTION_API UFoliageRTInvokerComponent : public USceneComponent
{
    GENERATED_BODY()

public:

    UFoliageRTInvokerComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

    /** Single interaction brush material.
     *  Outputs RGBA16f packed: R=Pack(PushDirX,VelX) G=Pack(PushDirY,VelY) B=Pack(HeightMask,Progression) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush")
    TObjectPtr<UMaterialInterface> BrushMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush",
        meta = (ClampMin = 1.f))
    float BrushRadius = 60.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush",
        meta = (ClampMin = 0.f, ClampMax = 1.f))
    float BrushWeight = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush",
        meta = (ClampMin = 0.f, ClampMax = 1.f))
    float BrushSoftness = 0.5f;
    

    FRTInvokerFrameData GetFrameData(int32 SlotIndex, FVector2D CellOriginWS, float CellSize) const;

    UFUNCTION(BlueprintPure, Category = "Foliage RT")
    UMaterialInstanceDynamic* GetMID_Interaction() const { return MID_Interaction; }

    UPROPERTY(BlueprintReadOnly, Category = "Foliage RT|State")
    FVector2D EncodedVelocity = FVector2D(0.5f, 0.5f);

private:

    FVector2D ComputeVelocity(float DeltaTime) const;
    float     EncodeVelocityAxis(float WorldVelAxis) const;

    UPROPERTY(Transient)
    TObjectPtr<URTPoolManager> PoolManager;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> MID_Interaction;

    FVector PrevWorldLocation = FVector::ZeroVector;
    bool    bFirstTick        = true;

    // Cached from settings at BeginPlay — shared across all invokers
    float CachedMaxVelocity = 1200.f;

    float CachedDecayRate   = 0.95f;
};