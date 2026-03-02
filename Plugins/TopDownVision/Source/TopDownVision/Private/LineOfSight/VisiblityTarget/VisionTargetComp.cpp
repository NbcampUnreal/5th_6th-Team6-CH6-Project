// Fill out your copyright notice in the Description page of Project Settings.

//Class
#include "LineOfSight//VisiblityTarget/VisionTargetComp.h"

//comps
#include "LineOfSight/LOSVisual/LOSStampDrawerComp.h"
#include "LineOfSight/WorldObstacle/LOSObstacleDrawerComponent.h"
#include "LineOfSight/LOSVisual/VisibilityMeshComp.h"

//Debug
#include "TopDownVisionDebug.h"



UVisionTargetComp::UVisionTargetComp()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    ObstacleDrawer = CreateDefaultSubobject<ULOSObstacleDrawerComponent>(TEXT("ObstacleDrawer"));
    StampDrawer      = CreateDefaultSubobject<ULOSStampDrawerComp>(TEXT("StampDrawer"));
    VisibilityMesh   = CreateDefaultSubobject<UVisibilityMeshComp>(TEXT("VisibilityMesh"));
}

void UVisionTargetComp::BeginPlay()
{
    Super::BeginPlay();

    if (!ShouldRunClientLogic())
        return;

    // Pass MaxVisionRange into ObstacleDrawer so it can size its RT correctly
    if (ObstacleDrawer)
        ObstacleDrawer->Initialize(MaxVisionRange);

    // Pass initial range into StampDrawer for material param
    if (StampDrawer)
        StampDrawer->OnVisionRangeChanged(VisionRange, MaxVisionRange);

    // Create MIDs from whatever meshes were registered in editor or via AddMesh
    if (VisibilityMesh)
        VisibilityMesh->Initialize();
}

// -------------------------------------------------------------------------- //
//  Update
// -------------------------------------------------------------------------- //

void UVisionTargetComp::UpdateVision()
{
    if (!ShouldRunClientLogic())
        return;

    if (ObstacleDrawer)
        ObstacleDrawer->UpdateObstacleTexture();

    if (StampDrawer)
        StampDrawer->UpdateLOSStamp(
            ObstacleDrawer ? ObstacleDrawer->GetObstacleRenderTarget() : nullptr);
}

void UVisionTargetComp::ToggleLOSStampUpdate(bool bIsOn)
{
    if (StampDrawer)
        StampDrawer->ToggleLOSStampUpdate(bIsOn);
}

bool UVisionTargetComp::IsUpdating() const
{
    return StampDrawer ? StampDrawer->IsUpdating() : false;
}

// -------------------------------------------------------------------------- //
//  Visibility Fade
// -------------------------------------------------------------------------- //

void UVisionTargetComp::SetVisible(bool bVisible)
{
    TargetVisibilityAlpha = bVisible ? 1.0f : 0.0f;

    if (!GetWorld()->GetTimerManager().IsTimerActive(FadeTimerHandle))
    {
        GetWorld()->GetTimerManager().SetTimer(
            FadeTimerHandle, this,
            &UVisionTargetComp::UpdateVisibilityFade,
            0.016f, true);
    }
}

void UVisionTargetComp::UpdateVisibilityFade()
{
    VisibilityAlpha = FMath::FInterpTo(VisibilityAlpha, TargetVisibilityAlpha, 0.016f, FadeSpeed);

    if (VisibilityMesh)
        VisibilityMesh->UpdateVisibility(VisibilityAlpha);

    if (FMath::IsNearlyEqual(VisibilityAlpha, TargetVisibilityAlpha, 0.005f))
    {
        VisibilityAlpha = TargetVisibilityAlpha;
        GetWorld()->GetTimerManager().ClearTimer(FadeTimerHandle);

        UE_LOG(LOSVision, Verbose,
            TEXT("[%s] UVisionTargetComp::UpdateVisibilityFade >> Complete: %.2f"),
            *GetOwner()->GetName(), VisibilityAlpha);
    }
}

// -------------------------------------------------------------------------- //
//  Range
// -------------------------------------------------------------------------- //

void UVisionTargetComp::SetVisionRange(float NewRange)
{
    VisionRange = FMath::Clamp(NewRange, 0.f, MaxVisionRange);

    if (StampDrawer)
        StampDrawer->OnVisionRangeChanged(VisionRange, MaxVisionRange);

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] UVisionTargetComp::SetVisionRange >> %.1f"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        VisionRange);
}

UMaterialInstanceDynamic* UVisionTargetComp::GetStampMID() const
{
    return StampDrawer ? StampDrawer->GetLOSMaterialMID() : nullptr;
}

// -------------------------------------------------------------------------- //

bool UVisionTargetComp::ShouldRunClientLogic() const
{
    return GetNetMode() != NM_DedicatedServer;
}
