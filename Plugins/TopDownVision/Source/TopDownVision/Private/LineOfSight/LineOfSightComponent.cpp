// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/LineOfSightComponent.h"
#include "Engine/World.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TopDownVisionLogCategories.h"// log
#include "Components/SceneCaptureComponent2D.h"

ULineOfSightComponent::ULineOfSightComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // Manual draw

    //Create 2DSceneCaptureComp
    SceneCaptureComp = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("LOS_SceneCapture"));
    SceneCaptureComp->SetupAttachment(this);//attach to the owner's root
}

void ULineOfSightComponent::BeginPlay()
{
    Super::BeginPlay();

    CreateResources();// make CRT and MID
}

void ULineOfSightComponent::UpdateLocalLOS()
{
    if (!ShouldUpdate)
    {
        UE_LOG(LOSVision, Log,
            TEXT("ULineOfSightComponent::UpdateLocalLOS >> Skipped, ShouldUpdate is false"));
        return;
    }

    if (!SceneCaptureComp)
    {
        UE_LOG(LOSVision, Error,
            TEXT("ULineOfSightComponent::UpdateLocalLOS >> Invalid SceneCaptureComp"));
        return;
    }
    
    if (!CanvasRenderTarget)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("ULineOfSightComponent::UpdateLocalLOS >> Invalid CanvasRenderTarget"));
        return;
    }
    
    SceneCaptureComp->CaptureScene();// Capture the scene
    CanvasRenderTarget->UpdateResource(); // triggers DrawLOS
   
    
    UE_LOG(LOSVision, Log,
        TEXT("ULineOfSightComponent::UpdateLocalLOS >> UpdateResource called"));
}

void ULineOfSightComponent::UpdateVisibleRange(float NewRange)
{
    VisionRange = FMath::Max(0.f, NewRange); // clamp to non-negative

    UE_LOG(LOSVision, Log,
        TEXT("ULineOfSightComponent::UpdateVisibleRange >> VisionRange set to %f"),
        VisionRange);
}


void ULineOfSightComponent::ToggleUpdate(bool bIsOn)
{
    if (ShouldUpdate==bIsOn)
    {
        UE_LOG(LOSVision, Log,
            TEXT("ULineOfSightComponent::ToggleUpdate >> Already set to %s"),
            ShouldUpdate ? TEXT("true") : TEXT("false"));
        return;
    }
    
    ShouldUpdate = bIsOn;

    UE_LOG(LOSVision, Log,
        TEXT("ULineOfSightComponent::ToggleUpdate >> ShouldUpdate set to %s"),
        ShouldUpdate ? TEXT("true") : TEXT("false"));
}

void ULineOfSightComponent::DrawLOS(UCanvas* Canvas, int32 Width, int32 Height)
{
    if (!Canvas || !LOSMaterialMID)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("ULineOfSightComponent::DrawLOS >> Canvas or LOSMaterialMID is null"));
        return;
    }

    FCanvasTileItem Tile(
        FVector2D(0, 0),
        LOSMaterialMID->GetRenderProxy(),
        FVector2D(Width, Height)
    );

    Tile.BlendMode = SE_BLEND_Opaque; // full mask
    Canvas->DrawItem(Tile);

    UE_LOG(LOSVision, Log,
        TEXT("ULineOfSightComponent::DrawLOS >> MID material drawn to Canvas"));
}

void ULineOfSightComponent::CreateResources()
{
    if (!GetWorld())
        return;

    // Create dynamic LOS render target
    CanvasRenderTarget = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(
        GetWorld(),
        UCanvasRenderTarget2D::StaticClass(),
        PixelResolution,
        PixelResolution);

    if (!CanvasRenderTarget)
    {
        UE_LOG(LOSVision, Warning,
                 TEXT("ULineOfSightComponent::PrepareDynamics >> Failed to create CanvasRenderTarget"));
        return;
    }
    
    //add dynamic for updating LOSdraw
    CanvasRenderTarget->OnCanvasRenderTargetUpdate.AddDynamic(this, &ULineOfSightComponent::DrawLOS);
    UE_LOG(LOSVision, Log,
        TEXT("ULineOfSightComponent::PrepareDynamics >> CanvasRenderTarget created"));

    SceneCaptureComp->TextureTarget = CanvasRenderTarget;//draw on newly created CRT
    
    // Create MID from the base material so that it can be drawn and be used as texture obj
    if (!LOSMaterial)
    {
        UE_LOG(LOSVision, Warning,
                TEXT("ULineOfSightComponent::PrepareDynamics >> Invalid Material for Making MID"));
        return;
    }
    
    LOSMaterialMID = UMaterialInstanceDynamic::Create(LOSMaterial, this);
    UE_LOG(LOSVision, Log,
        TEXT("ULineOfSightComponent::PrepareDynamics >> LOSMaterialMID created"));

    LOSMaterialMID->SetTextureParameterValue(
        MIDTextureParam,
        CanvasRenderTarget);// pass the CRT to the MID
}


