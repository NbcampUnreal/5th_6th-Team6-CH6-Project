#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TopDown2DShapeComp.generated.h"

UENUM()
enum E2DShapeType : uint8
{
    None       = 255 UMETA(DisplayName = "None"),
    
    Circle     = 0   UMETA(DisplayName = "Circle"),
    Square     = 1   UMETA(DisplayName = "Square"),
    FreePoints = 2   UMETA(DisplayName = "FreePoints"),
};


//FD
class USplineComponent;


//Logs
TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(TopDown2DShape, Log, All);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UTopDown2DShapeComp : public USceneComponent
{
    GENERATED_BODY()

public:
    UTopDown2DShapeComp();

public:
    UFUNCTION(BlueprintCallable, Category="2DShapeComp")
    void GenerateSamplePoints();// this will be called in the editor and be saved for later

    const TArray<FVector2D>& GetInnerSamplePoints()   const { return InnerSamplePoints; }
    const TArray<FVector2D>& GetOutlineSamplePoints() const { return OutLineSamplePoints; }

    float GetTraceRadius() const;
    TEnumAsByte<E2DShapeType> GetShapeType() const { return ShapeType; }

    UFUNCTION(BlueprintCallable, Category="2DShapeComp")
    void DrawDebugShape(float Duration = 0.f) const;

    //Editor Setter function for spline
    UFUNCTION(BlueprintCallable, Category="2DShapeComp")//!!! not for gameplay runtime!!!!
    void SetSplineComp(USplineComponent* InSplineComp);//Spline Setter (need to be called before generation)

    float GetPointSpacingDistance() const { return PointSpacingDistance; }// for shape aware fan making
    
private:
    //circle
    void GenerateInnerSamplePoints_Circle();
    void GenerateOutlineSamplePoints_Circle();
    //Square
    void GenerateInnerSamplePoints_Square();
    void GenerateOutlineSamplePoints_Square();
    //FreePoints -> Spline
    void GenerateInnerSamplePoints_FreePoints();
    void GenerateOutlineSamplePoints_FreePoints();

   

    //internal evaluation for point population
    bool IsPointInsidePolygon(const FVector2D& Point) const;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="2DShapeComp")
    TEnumAsByte<E2DShapeType> ShapeType = E2DShapeType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="2DShapeComp", meta=(ClampMin="1.0"))// min for safety
    float PointSpacingDistance = 10.f;

    // Circle
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="2DShapeComp|Circle",
        meta=(EditCondition="ShapeType == E2DShapeType::Circle", EditConditionHides))
    float Radius = 50.f;

    // Square
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="2DShapeComp|Square",
        meta=(EditCondition="ShapeType == E2DShapeType::Square", EditConditionHides))
    float BoxWidth = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="2DShapeComp|Square",
        meta=(EditCondition="ShapeType == E2DShapeType::Square", EditConditionHides))
    float BoxHeight = 100.f;

    // FreePoints — spline comp goes here when ready

    UPROPERTY(VisibleAnywhere, Category="2DShapeComp|Output")
    TArray<FVector2D> OutLineSamplePoints;

    UPROPERTY(VisibleAnywhere, Category="2DShapeComp|Output")
    TArray<FVector2D> InnerSamplePoints;

    // --- Debug --- //
    UPROPERTY(EditAnywhere, Category="2DShapeComp|Debug")
    FColor ShapeBoundaryColor = FColor::Cyan;

    UPROPERTY(EditAnywhere, Category="2DShapeComp|Debug")
    FColor InnerPointColor = FColor::Green;

    UPROPERTY(EditAnywhere, Category="2DShapeComp|Debug")
    FColor OutlinePointColor = FColor::Yellow;

    UPROPERTY(EditAnywhere, Category="2DShapeComp|Debug", meta=(ClampMin="1.0"))
    float DebugPointSize = 4.f;

    UPROPERTY(EditAnywhere, Category="2DShapeComp|Debug", meta=(ClampMin="1.0"))
    float DebugLineThickness = 1.f;

private:
    UPROPERTY()
    USplineComponent* ShapeSplineComp = nullptr;//settled spline comp from outside
};