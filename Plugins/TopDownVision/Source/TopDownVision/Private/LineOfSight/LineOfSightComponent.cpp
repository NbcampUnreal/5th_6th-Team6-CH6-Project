// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/LineOfSightComponent.h"
#include "Engine/World.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TopDownVisionLogCategories.h"// log

ULineOfSightComponent::ULineOfSightComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // Manual draw
}

void ULineOfSightComponent::BeginPlay()
{
    Super::BeginPlay();

    PrepareDynamics();// make CRT and MID
}

void ULineOfSightComponent::UpdateLocalLOS()
{
    if (!CanvasRenderTarget)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("ULineOfSightComponent::UpdateLocalLOS >> CanvasRenderTarget is null"));
        return;
    }

    if (!LOSMaterialMID)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("ULineOfSightComponent::UpdateLocalLOS >> LOSMaterialMID is null"));
        return;
    }

    bDirty = true;
    CanvasRenderTarget->UpdateResource(); // triggers DrawLOS
    UE_LOG(LOSVision, Log,
        TEXT("ULineOfSightComponent::UpdateLocalLOS >> UpdateResource called"));
}

void ULineOfSightComponent::UpdateVisibleRange(float NewRange)
{
    if (!LOSMaterialMID)// now update to the MID, not MPC
    {
        UE_LOG(LOSVision, Warning,
            TEXT("ULineOfSightComponent::UpdateVisibleRange >> LOSMaterialMID is null"));
        return;
    }
    VisionRange = FMath::Max(0.f, NewRange); // clamp to non-negative
    
    LOSMaterialMID->SetScalarParameterValue(VisibleRangeScalarValueName, NewRange);
    UE_LOG(LOSVision, Log,
        TEXT("ULineOfSightComponent::UpdateVisibleRange >> MID updated Range[%f]"),
        NewRange);
}

void ULineOfSightComponent::UpdateCurrentLocation()
{
    if (!LOSMaterialMID || !GetOwner())
    {
        UE_LOG(LOSVision, Warning,
            TEXT("ULineOfSightComponent::UpdateCurrentLocation >> LOSMaterialMID or Owner is null"));
        return;
    }

    LOSMaterialMID->SetVectorParameterValue(LocationVectorValueName, GetOwner()->GetActorLocation());
    UE_LOG(LOSVision, Log,
        TEXT("ULineOfSightComponent::UpdateCurrentLocation >> MID updated ActorLocation (%s)"),
        *GetOwner()->GetActorLocation().ToString());
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

    bDirty = false;
}

void ULineOfSightComponent::PrepareDynamics()
{
    if (!CanvasRenderTarget && GetWorld())
    {
        CanvasRenderTarget = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(
            GetWorld(),
            UCanvasRenderTarget2D::StaticClass(),
            PixelResolution,
            PixelResolution
        );

        if (CanvasRenderTarget)
        {
            CanvasRenderTarget->OnCanvasRenderTargetUpdate.AddDynamic(this, &ULineOfSightComponent::DrawLOS);
            UE_LOG(LOSVision, Log,
                TEXT("ULineOfSightComponent::PrepareDynamics >> CanvasRenderTarget created"));
        }
        else
        {
            UE_LOG(LOSVision, Warning,
                TEXT("ULineOfSightComponent::PrepareDynamics >> Failed to create CanvasRenderTarget"));
        }
    }

    // Create MID from the base material so that it can be drawn and be used as texture obj
    if (LOSMaterial && !LOSMaterialMID)
    {
        LOSMaterialMID = UMaterialInstanceDynamic::Create(LOSMaterial, this);
        UE_LOG(LOSVision, Log,
            TEXT("ULineOfSightComponent::PrepareDynamics >> LOSMaterialMID created"));
    }
}


