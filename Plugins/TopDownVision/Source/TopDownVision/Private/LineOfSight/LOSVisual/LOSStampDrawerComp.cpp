// Fill out your copyright notice in the Description page of Project Settings.

#include "LineOfSight/LOSVisual/LOSStampDrawerComp.h"

#include "Engine/World.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "DrawDebugHelpers.h"
#include "TopDownVisionDebug.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"


ULOSStampDrawerComp::ULOSStampDrawerComp()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void ULOSStampDrawerComp::BeginPlay()
{
    Super::BeginPlay();
   // CreateResources(); -> do this outside of this comp manually
}

// -------------------------------------------------------------------------- //

void ULOSStampDrawerComp::CreateResources()
{
    if (!GetWorld())
        return;

    if (!LOSRenderTarget) 
    {
        FString RTName = FString::Printf(TEXT("LOSStampRT_%s"), *GetOwner()->GetName());
        LOSRenderTarget = NewObject<UTextureRenderTarget2D>(this, FName(*RTName));
        LOSRenderTarget->InitAutoFormat(PixelResolution, PixelResolution);
        LOSRenderTarget->ClearColor        = FLinearColor::Black;
        LOSRenderTarget->RenderTargetFormat = RTF_R8;

        UE_LOG(LOSVision, Log,
            TEXT("[%s] ULOSStampDrawerComp::CreateResources >> RT: %s (%p)"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()),
            *LOSRenderTarget->GetName(), LOSRenderTarget);
    }

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
    }
}


// -------------------------------------------------------------------------- //

void ULOSStampDrawerComp::UpdateLOSStamp(UTextureRenderTarget2D* ObstacleRT)
{
   if (!bShouldUpdateLOSStamp)
        return;

    if (!LOSRenderTarget)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("[%s] ULOSStampDrawerComp::UpdateLOSStamp >> LOSRenderTarget is null"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    if (!LOSMaterialMID)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("[%s] ULOSStampDrawerComp::UpdateLOSStamp >> LOSMaterialMID is null"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    // Log what we're about to draw with
    UE_LOG(LOSVision, Log,
        TEXT("[%s] ULOSStampDrawerComp::UpdateLOSStamp >> ObstacleRT=%s MID=%s"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        ObstacleRT ? *ObstacleRT->GetName() : TEXT("NULL"),
        *LOSMaterialMID->GetName());

    if (ObstacleRT && MIDObstacleTextureParam != NAME_None)
        LOSMaterialMID->SetTextureParameterValue(MIDObstacleTextureParam, ObstacleRT);
    else
        UE_LOG(LOSVision, Warning,
            TEXT("[%s] ULOSStampDrawerComp::UpdateLOSStamp >> ObstacleRT null or param not set — stamp will be empty"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));

    {
        UCanvas* Canvas = nullptr;
        FDrawToRenderTargetContext Context;
        FVector2D RTSize(LOSRenderTarget->SizeX, LOSRenderTarget->SizeY);

        UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(
            GetWorld(), LOSRenderTarget, Canvas, RTSize, Context);

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
            
            UE_LOG(LOSVision, Verbose,
                TEXT("[%s] ULOSStampDrawerComp::UpdateLOSStamp >> Drew to LOSRenderTarget successfully"),
                *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        }
        else
        {
            UE_LOG(LOSVision, Warning,
                TEXT("[%s] ULOSStampDrawerComp::UpdateLOSStamp >> Failed to get canvas"),
                *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        }
    }

    // Debug copy — assign DebugRT in BP details to visualize stamp output
    if (bDrawDebugRT && DebugRT)
    {
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

    UE_LOG(LOSVision, Log,
        TEXT("[%s] ULOSStampDrawerComp::UpdateLOSStamp >> stamp updated"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()));
}

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

