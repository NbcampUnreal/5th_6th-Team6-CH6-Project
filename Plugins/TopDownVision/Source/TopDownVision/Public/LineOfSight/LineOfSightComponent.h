#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "LineOfSightComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API ULineOfSightComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULineOfSightComponent();

protected:
    virtual void BeginPlay() override;
    virtual void OnComponentCreated() override;

public:
    /** Manual call to draw LOS */
    UFUNCTION(BlueprintCallable, Category="LineOfSight")
    void DrawLineOfSight();

private:
    UFUNCTION()
    void UpdateRenderTarget(UCanvas* Canvas, int32 Width, int32 Height);

protected:
    /** Vision range */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    float VisionRange = 500.f;

    /** Number of radial rays */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    int32 RayCounts = 100;

    /** Canvas render target to draw LOS */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    UCanvasRenderTarget2D* CanvasRenderTarget;

    /** Material parameter collection for actor location */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOS")
    UMaterialParameterCollection* VisionMPC;

    /** Trace channel for LOS */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOS")
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

    /** Actors to ignore */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOS")
    TArray<AActor*> IgnoringActors;

    /** Components to ignore */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOS")
    TArray<UPrimitiveComponent*> IgnoringComponents;

    /** White texture fallback (generated automatically) */
    UPROPERTY(Transient)
    UTexture2D* WhiteTexture;
};
