
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "UI_AMiniMapCapture.generated.h"

UCLASS()
class PROJECTER_API AUI_AMiniMapCapture : public AActor
{
	GENERATED_BODY()
	
public:
    AUI_AMiniMapCapture();
    
    UFUNCTION()
    void UpdateMiniMap();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Minimap")
    class USceneComponent* RootScene; // 추가

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Minimap")
    class USceneCaptureComponent2D* CaptureComponent;

    UPROPERTY(EditAnywhere)
    class UTextureRenderTarget2D* MapRenderTarget;

};
