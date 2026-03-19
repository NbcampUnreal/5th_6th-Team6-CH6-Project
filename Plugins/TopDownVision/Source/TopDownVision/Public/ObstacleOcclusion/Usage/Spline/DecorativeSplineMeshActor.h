#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineMeshComponent.h"
#include "DecorativeSplineMeshActor.generated.h"

class USplineComponent;
class UStaticMesh;

UCLASS()
class TOPDOWNVISION_API ADecorativeSplineMeshActor : public AActor
{
    GENERATED_BODY()

public:

    ADecorativeSplineMeshActor();

    virtual void BeginPlay() override;

    UFUNCTION(CallInEditor, BlueprintCallable, Category="Spline Mesh")
    void BuildSplineMesh();

    // Returns all generated spline mesh segments — used by occlusion system
    const TArray<TObjectPtr<USplineMeshComponent>>& GetSplineMeshComponents() const { return SplineMeshComponents; }

protected:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spline Mesh")
    TObjectPtr<USplineComponent> SplineComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spline Mesh")
    TObjectPtr<UStaticMesh> SplineMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spline Mesh")
    float SectionLength = 200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spline Mesh")
    float ScaleRatio = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spline Mesh")
    bool bShouldFullLength = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spline Mesh")
    bool bShouldAllowScaling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spline Mesh")
    TEnumAsByte<ESplineMeshAxis::Type> ForwardAxis = ESplineMeshAxis::X;

    // Tag applied to all generated segments — used by occlusion DiscoverChildMeshes
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spline Mesh|Occlusion")
    FName SegmentOcclusionTag = TEXT("OcclusionMesh");

private:

    void ClearSplineMeshComponents();
    void GenerateSegment(int32 SegmentIndex);

    void GetLocationAndTangent(
        int32 SegmentIndex,
        bool bIsStart,
        FVector& OutLocation,
        FVector& OutTangent) const;

    // All generated segments — rebuilt on each BuildSplineMesh call
    UPROPERTY(VisibleAnywhere, Category="Spline Mesh")
    TArray<TObjectPtr<USplineMeshComponent>> SplineMeshComponents;
};