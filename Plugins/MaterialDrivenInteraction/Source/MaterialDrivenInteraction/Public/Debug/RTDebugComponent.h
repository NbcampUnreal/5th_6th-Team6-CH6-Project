#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RTDebugComponent.generated.h"

class URTPoolManager;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UStaticMesh;
class UMaterialInterface;

UENUM(BlueprintType)
enum class ERTDebugChannel : uint8
{
    PushDirection = 0   UMETA(DisplayName = "Push Direction"),
    Velocity      = 1   UMETA(DisplayName = "Velocity"),
    HeightMask    = 2   UMETA(DisplayName = "Height Mask"),
    Progression   = 3   UMETA(DisplayName = "Progression"),
    
    None          = 255 UMETA(Hidden)
};

UCLASS(ClassGroup = "Foliage RT", meta = (BlueprintSpawnableComponent))
class MATERIALDRIVENINTERACTION_API URTDebugComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    URTDebugComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Debug")
    bool bShowDebugBoxes = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Debug")
    bool bShowDebugPlanes = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Debug")
    ERTDebugChannel DebugChannel = ERTDebugChannel::PushDirection;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Debug")
    float PlaneHeight = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Debug")
    FColor ActiveBoxColor = FColor(100, 220, 255);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Debug")
    TObjectPtr<UMaterialInterface> DebugPlaneMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Debug")
    TObjectPtr<UStaticMesh> PlaneMesh;

    UFUNCTION(BlueprintCallable, Category = "Foliage RT|Debug")
    void RefreshDebugPlanes();

private:

    void DrawBoxes();
    void UpdatePlanes();
    void ResizePlanePool(int32 Count);
    void UpdatePlane(int32 PlaneIndex, int32 SlotIndex);
    void HidePlane(int32 PlaneIndex);

    UPROPERTY(Transient)
    TObjectPtr<URTPoolManager> PoolManager;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UStaticMeshComponent>> DebugPlanes;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> PlaneMIDs;

    static const FName PN_DebugTex;
    static const FName PN_DebugChannel;
};