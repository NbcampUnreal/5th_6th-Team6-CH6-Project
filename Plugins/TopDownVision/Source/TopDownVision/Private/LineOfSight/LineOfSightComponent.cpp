// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/LineOfSightComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TopDownVisionLogCategories.h"// log

//environment texture source
#include "LineOfSight/WorldObstacle/LocalTextureSampler.h"

#include "DrawDebugHelpers.h"//debug for visualizing the activation
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"


ULineOfSightComponent::ULineOfSightComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // Manual draw

    LocalTextureSampler = CreateDefaultSubobject<ULocalTextureSampler>(TEXT("LocalTextureSampler"));
}

void ULineOfSightComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!ShouldRunClientLogic())
    {
        return;// not for server
    }
    
    CreateResources();// make RT and MID

}

void ULineOfSightComponent::UpdateLocalLOS()
{
    if (!ShouldRunClientLogic())
    {
        return;// not for server
    }

    
    if (!ShouldUpdate)
    {
        UE_LOG(LOSVision, Verbose,
            TEXT("ULineOfSightComponent::UpdateLocalLOS >> Skipped, ShouldUpdate is false"));
        return;
    }
    if (!LOSRenderTarget)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("ULineOfSightComponent::UpdateLocalLOS >> Invalid HeightRenderTarget"));
        return;
    }
    // now only use local texture sampler as only a source of RT
    if (!LocalTextureSampler)
    {
        UE_LOG(LOSVision, Error,
            TEXT("ULineOfSightComponent::UpdateLocalLOS >> LocalTextureSampler missing"));
        return;
    }

    LocalTextureSampler->UpdateLocalTexture();

    //Debug
    if (bDrawTextureRange)//draw debug box for LOS stamp area
    {
        const FVector Center = GetOwner()->GetActorLocation();
        const FVector Extent = FVector(VisionRange, VisionRange, 50.f);

        DrawDebugBox(
            GetWorld(),
            Center,
            Extent,
            FQuat::Identity,
            FColor::Green,
            false,
            -1.f,
            0,
            2.f );
    }
    
    UE_LOG(LOSVision, Verbose,
        TEXT("ULineOfSightComponent::UpdateLocalLOS >> UpdateResource called"));
}

void ULineOfSightComponent::UpdateVisibleRange(float NewRange)
{
    
    const float OldRange = VisionRange;
    VisionRange = FMath::Max(0.f, NewRange); // clamp to non-negative
    
    if (!ShouldRunClientLogic())
    {
        return;
    }
    
    // Update material parameter if range changed
    if (!FMath::IsNearlyEqual(OldRange, VisionRange) && LOSMaterialMID)
    {
        LOSMaterialMID->SetScalarParameterValue(
            MIDVisibleRangeParam, 
            VisionRange / MaxVisionRange / 2.f);
        
        UE_LOG(LOSVision, Verbose,
            TEXT("ULineOfSightComponent::UpdateVisibleRange >> Updated material: VisionRange=%.1f, Normalized=%.3f"),
            VisionRange,
            VisionRange / MaxVisionRange / 2.f);
    }
    
    // Also update LocalTextureSampler's world sample radius
    if (LocalTextureSampler)
    {
        LocalTextureSampler->SetWorldSampleRadius(VisionRange);
    }
}


void ULineOfSightComponent::ToggleUpdate(bool bIsOn)
{
    if (ShouldUpdate==bIsOn)
    {
        UE_LOG(LOSVision, VeryVerbose,
            TEXT("ULineOfSightComponent::ToggleUpdate >> Already set to %s"),
            ShouldUpdate ? TEXT("true") : TEXT("false"));
        return;
    }
    
    ShouldUpdate = bIsOn;

    
    UE_LOG(LOSVision, VeryVerbose,
        TEXT("ULineOfSightComponent::ToggleUpdate >> ShouldUpdate set to %s"),
        ShouldUpdate ? TEXT("true") : TEXT("false"));
}


void ULineOfSightComponent::CreateResources()
{
    if (!ShouldRunClientLogic())
    {
        return;// not for server
    }
    
    if (!GetWorld())
        return;

    //make RT
    if (!LOSRenderTarget)
    {
        LOSRenderTarget = NewObject<UTextureRenderTarget2D>(this);
        LOSRenderTarget->InitAutoFormat(PixelResolution, PixelResolution);
        LOSRenderTarget->ClearColor = FLinearColor::Black;
    }
    
    LOSRenderTarget->RenderTargetFormat = RTF_R8; // only R/G needed
    
    if (!LocalTextureSampler)
    {
        UE_LOG(LOSVision, Error,
            TEXT("CreateResources >> LocalTextureSampler default subobject missing"));
        return;
    }

    LocalTextureSampler->SetLocalRenderTarget(LOSRenderTarget);
    LocalTextureSampler->SetLocationRoot(GetOwner()->GetRootComponent());
    LocalTextureSampler->SetWorldSampleRadius(VisionRange);
    // pass the owner's root so that it can know world location

    // Create MID
    if (LOSMaterial)
    {
        LOSMaterialMID = UMaterialInstanceDynamic::Create(LOSMaterial, this);
        if (LOSMaterialMID && LOSRenderTarget)
        {
            LOSMaterialMID->SetTextureParameterValue(MIDTextureParam, LOSRenderTarget);
            LOSMaterialMID->SetScalarParameterValue(MIDVisibleRangeParam, VisionRange / MaxVisionRange / 2.f);
        }
    }
}

bool ULineOfSightComponent::ShouldRunClientLogic() const
{
    if (GetNetMode() == NM_DedicatedServer)
        return false;
    
    //other conditions

    return true;
}


