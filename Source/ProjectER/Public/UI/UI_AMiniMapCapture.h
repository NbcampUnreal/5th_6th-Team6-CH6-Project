
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

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere)
    class USceneCaptureComponent2D* CaptureComponent;

    UPROPERTY(EditAnywhere)
    class UTextureRenderTarget2D* MapRenderTarget;

};
