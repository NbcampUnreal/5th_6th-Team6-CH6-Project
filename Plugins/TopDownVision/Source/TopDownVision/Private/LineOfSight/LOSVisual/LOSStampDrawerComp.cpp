// Fill out your copyright notice in the Description page of Project Settings.

#include "LineOfSight/LOSVisual/LOSStampDrawerComp.h"

#include "Engine/World.h"
#include "Engine/Canvas.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "DrawDebugHelpers.h"
#include "TopDownVisionDebug.h"


ULOSStampDrawerComp::ULOSStampDrawerComp()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void ULOSStampDrawerComp::BeginPlay()
{
    Super::BeginPlay();
    // CreateResources() is called manually from Vision_VisualComp::BeginPlay
}

// -------------------------------------------------------------------------- //

void ULOSStampDrawerComp::CreateResources()
{
    if (!GetWorld())
        return;

    // No RT needed — MainVisionRTManager draws the MID directly onto the camera RT.
    // Only the MID needs to be created here.
    if (LOSMaterial)
    {
        FString MIDName = FString::Printf(TEXT("LOSStampMID_%s"), *GetOwner()->GetName());
        LOSMaterialMID = UMaterialInstanceDynamic::Create(LOSMaterial, this, FName(*MIDName));

        if (LOSMaterialMID)
        {
            UE_LOG(LOSVision, Log,
                TEXT("[%s] ULOSStampDrawerComp::CreateResources >> MID: %s (%p)"),
                *TopDownVisionDebug::GetClientDebugName(GetOwner()),
                *LOSMaterialMID->GetName(), LOSMaterialMID);
        }
        else
        {
            UE_LOG(LOSVision, Warning,
                TEXT("[%s] ULOSStampDrawerComp::CreateResources >> Failed to create MID"),
                *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        }
    }
    else
    {
        UE_LOG(LOSVision, Warning,
            TEXT("[%s] ULOSStampDrawerComp::CreateResources >> LOSMaterial not assigned"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
    }
}

// -------------------------------------------------------------------------- //

void ULOSStampDrawerComp::UpdateLOSStamp(UTextureRenderTarget2D* ObstacleRT)
{
    if (!bShouldUpdateLOSStamp)
        return;

    if (!LOSMaterialMID)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("[%s] ULOSStampDrawerComp::UpdateLOSStamp >> LOSMaterialMID is null"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    // Bind obstacle RT to the MID each frame.
    // MainVisionRTManager::DrawLOSStamp reads GetLOSMaterialMID() and
    // draws it directly onto the camera RT — no pre-render step here.
    if (ObstacleRT && MIDObstacleTextureParam != NAME_None)
    {
        LOSMaterialMID->SetTextureParameterValue(MIDObstacleTextureParam, ObstacleRT);
    }
    else
    {
        UE_LOG(LOSVision, Warning,
            TEXT("[%s] ULOSStampDrawerComp::UpdateLOSStamp >> ObstacleRT null or param not set — stamp will be empty"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
    }

    // Debug copy — assign a RT asset in BP details to visualize the MID output
    if (bDrawDebugRT && DebugRT)
    {
        UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), DebugRT, FLinearColor::Black);

        UCanvas* Canvas = nullptr;
        FDrawToRenderTargetContext Context;
        FVector2D RTSize(DebugRT->SizeX, DebugRT->SizeY);

        UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(
            GetWorld(), DebugRT, Canvas, RTSize, Context);

        if (Canvas)
        {
            FCanvasTileItem Tile(
                FVector2D(0, 0),
                LOSMaterialMID->GetRenderProxy(),
                RTSize
            );
            Tile.BlendMode = SE_BLEND_Opaque;
            Canvas->DrawItem(Tile);

            UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);
        }
    }

    if (bDrawStampRange)
    {
        DrawDebugBox(GetWorld(), GetOwner()->GetActorLocation(),
            FVector(CachedVisionRange, CachedVisionRange, 50.f),
            FQuat::Identity, FColor::Blue, false, -1.f, 0, 2.f);
    }
}

// -------------------------------------------------------------------------- //

void ULOSStampDrawerComp::ToggleLOSStampUpdate(bool bIsOn)
{
    if (bShouldUpdateLOSStamp == bIsOn)
        return;

    bShouldUpdateLOSStamp = bIsOn;

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] ULOSStampDrawerComp::ToggleLOSStampUpdate >> %s"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        bIsOn ? TEXT("ON") : TEXT("OFF"));
}

void ULOSStampDrawerComp::OnVisionRangeChanged(float NewRange, float MaxRange)
{
    if (LOSMaterialMID && MIDVisibleRangeParam != NAME_None)
    {
        const float Normalized = FMath::Max(0.f, NewRange) / FMath::Max(1.f, MaxRange) / 2.f;
        LOSMaterialMID->SetScalarParameterValue(MIDVisibleRangeParam, Normalized);
        CachedVisionRange = NewRange;
    }
}
