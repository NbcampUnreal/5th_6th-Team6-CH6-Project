// Fill out your copyright notice in the Description page of Project Settings.

#include "LineOfSight/LOSVisual/LOSStampDrawerComp.h"

#include "Engine/World.h"
#include "Engine/Canvas.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "DrawDebugHelpers.h"

#include "EditorSetting/LOSResourcePoolSettings.h"
#include "TopDownVisionDebug.h"


ULOSStampDrawerComp::ULOSStampDrawerComp()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void ULOSStampDrawerComp::BeginPlay()
{
    Super::BeginPlay();
    // CreateResources() called manually from Vision_VisualComp::Initialize (owned mode only)
}

// -------------------------------------------------------------------------- //
//  Owned mode — resource creation
// -------------------------------------------------------------------------- //

void ULOSStampDrawerComp::CreateResources()
{
    if (!GetWorld())
        return;

    if (!LOSMaterial)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("[%s] ULOSStampDrawerComp::CreateResources >> LOSMaterial not assigned — owned mode requires it"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    FString MIDName = FString::Printf(TEXT("LOSStampMID_%s"), *GetOwner()->GetName());
    LOSMaterialMID = UMaterialInstanceDynamic::Create(LOSMaterial, this, FName(*MIDName));

    if (LOSMaterialMID)
    {
        UE_LOG(LOSVision, Verbose,
            TEXT("[%s] ULOSStampDrawerComp::CreateResources >> MID created: %s (%p)"),
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

// -------------------------------------------------------------------------- //
//  Pool mode — MID injection
// -------------------------------------------------------------------------- //

void ULOSStampDrawerComp::SetStampMID(UMaterialInstanceDynamic* InMID)
{
    LOSMaterialMID = InMID;

    if (InMID)
    {
        UE_LOG(LOSVision, Verbose,
            TEXT("[%s] ULOSStampDrawerComp::SetStampMID >> MID bound (%p)"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()), InMID);
    }
    else
    {
        UE_LOG(LOSVision, Verbose,
            TEXT("[%s] ULOSStampDrawerComp::SetStampMID >> MID cleared (slot released)"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
    }
}

// -------------------------------------------------------------------------- //
//  Update
// -------------------------------------------------------------------------- //

void ULOSStampDrawerComp::UpdateLOSStamp(UTextureRenderTarget2D* ObstacleRT)
{
    if (!bShouldUpdateLOSStamp)
        return;

    if (!LOSMaterialMID)
    {
        UE_LOG(LOSVision, Verbose,
            TEXT("[%s] ULOSStampDrawerComp::UpdateLOSStamp >> MID is null — skipping (slot not yet acquired?)"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    // Read param names from settings — single source of truth for both modes
    const ULOSResourcePoolSettings* Settings = ULOSResourcePoolSettings::Get();

    if (Settings && Settings->StampVisibilityAlphaParam != NAME_None)
        LOSMaterialMID->SetScalarParameterValue(Settings->StampVisibilityAlphaParam, PassedVisionAlpha);

    if (ObstacleRT && Settings && Settings->StampObstacleTextureParam != NAME_None)
    {
        LOSMaterialMID->SetTextureParameterValue(Settings->StampObstacleTextureParam, ObstacleRT);
    }
    else if (!ObstacleRT)
    {
        UE_LOG(LOSVision, Verbose,
            TEXT("[%s] ULOSStampDrawerComp::UpdateLOSStamp >> ObstacleRT null — stamp will be empty this frame"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
    }

    // Debug copy
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
            FCanvasTileItem Tile(FVector2D(0, 0), LOSMaterialMID->GetRenderProxy(), RTSize);
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
    if (!LOSMaterialMID)
        return;

    const ULOSResourcePoolSettings* Settings = ULOSResourcePoolSettings::Get();
    if (!Settings)
        return;

    if (Settings->StampNormalizedRadiusParam != NAME_None)
    {
        const float Normalized = FMath::Max(0.f, NewRange) / FMath::Max(1.f, MaxRange) / 2.f;
        LOSMaterialMID->SetScalarParameterValue(Settings->StampNormalizedRadiusParam, Normalized);
    }

    if (Settings->StampVisibilityAlphaParam != NAME_None)
        LOSMaterialMID->SetScalarParameterValue(Settings->StampVisibilityAlphaParam, 1.f);

    CachedVisionRange = NewRange;
}
