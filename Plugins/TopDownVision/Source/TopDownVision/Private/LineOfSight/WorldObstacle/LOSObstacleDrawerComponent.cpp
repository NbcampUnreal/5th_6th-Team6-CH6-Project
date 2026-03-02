// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/WorldObstacle/LOSObstacleDrawerComponent.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

#include "LineOfSight/WorldObstacle/LocalTextureSampler.h"
#include "TopDownVisionDebug.h"


ULOSObstacleDrawerComponent::ULOSObstacleDrawerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    LocalTextureSampler = CreateDefaultSubobject<ULocalTextureSampler>(TEXT("LocalTextureSampler"));
}

void ULOSObstacleDrawerComponent::BeginPlay()
{
    Super::BeginPlay();
    // CreateResources is called via Initialize() from VisionTargetComp::BeginPlay
}

// -------------------------------------------------------------------------- //

void ULOSObstacleDrawerComponent::Initialize(float InMaxVisionRange)
{
    MaxVisionRange = FMath::Max(1.f, InMaxVisionRange);
    CreateResources();

    UE_LOG(LOSVision, Log,
        TEXT("[%s] ULOSObstacleDrawerComponent::Initialize >> MaxVisionRange=%.1f"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        MaxVisionRange);
}

// -------------------------------------------------------------------------- //

void ULOSObstacleDrawerComponent::UpdateObstacleTexture()
{
    if (!ObstacleRenderTarget)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("[%s] ULOSObstacleDrawerComponent::UpdateObstacleTexture >> ObstacleRenderTarget is null"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    if (!LocalTextureSampler)
    {
        UE_LOG(LOSVision, Error,
            TEXT("[%s] ULOSObstacleDrawerComponent::UpdateObstacleTexture >> LocalTextureSampler missing"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    LocalTextureSampler->UpdateLocalTexture();

    if (bDrawSampleRange)
    {
        DrawDebugBox(
            GetWorld(),
            GetOwner()->GetActorLocation(),
            FVector(MaxVisionRange, MaxVisionRange, 50.f),
            FQuat::Identity,
            FColor::Green,
            false, -1.f, 0, 2.f);
    }

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] ULOSObstacleDrawerComponent::UpdateObstacleTexture >> sampled"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()));
}

// -------------------------------------------------------------------------- //

void ULOSObstacleDrawerComponent::CreateResources()
{
    if (!GetWorld())
        return;

    // ---- Render Target ---- //
    if (!ObstacleRenderTarget)
    {
        FString RTName = FString::Printf(TEXT("ObstacleRT_%s"), *GetOwner()->GetName());
        ObstacleRenderTarget = NewObject<UTextureRenderTarget2D>(this, FName(*RTName));
        ObstacleRenderTarget->InitAutoFormat(PixelResolution, PixelResolution);
        ObstacleRenderTarget->ClearColor         = FLinearColor::Black;
        ObstacleRenderTarget->RenderTargetFormat  = RTF_R8;
        ObstacleRenderTarget->UpdateResourceImmediate(); // flush to GPU before first draw

        UE_LOG(LOSVision, Log,
            TEXT("[%s] ULOSObstacleDrawerComponent::CreateResources >> RT created: %s (%p)"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()),
            *ObstacleRenderTarget->GetName(), ObstacleRenderTarget);
    }

    // ---- Local Texture Sampler ---- //
    if (LocalTextureSampler)
    {
        USceneComponent* BestRoot = nullptr;

        if (USceneComponent* OuterComp = Cast<USceneComponent>(GetOuter()))
            BestRoot = OuterComp;
        else
            BestRoot = GetOwner() ? GetOwner()->GetRootComponent() : nullptr;

        if (BestRoot)
        {
            LocalTextureSampler->SetLocationRoot(BestRoot);
        }
        else
        {
            UE_LOG(LOSVision, Error,
                TEXT("[%s] ULOSObstacleDrawerComponent::CreateResources >> Could not find valid root"),
                *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        }

        LocalTextureSampler->SetWorldSampleRadius(MaxVisionRange);
        LocalTextureSampler->SetLocalRenderTarget(ObstacleRenderTarget); // set last — triggers first draw
    }

    // ---- Material Instance Dynamic ---- //
    if (ObstacleMaterial)
    {
        FString MIDName = FString::Printf(TEXT("ObstacleMID_%s"), *GetOwner()->GetName());
        ObstacleMaterialMID = UMaterialInstanceDynamic::Create(ObstacleMaterial, this, FName(*MIDName));

        if (ObstacleMaterialMID && ObstacleRenderTarget && MIDTextureParam != NAME_None)
        {
            ObstacleMaterialMID->SetTextureParameterValue(MIDTextureParam, ObstacleRenderTarget);

            UE_LOG(LOSVision, Log,
                TEXT("[%s] ULOSObstacleDrawerComponent::CreateResources >> MID created: %s (%p)"),
                *TopDownVisionDebug::GetClientDebugName(GetOwner()),
                *ObstacleMaterialMID->GetName(), ObstacleMaterialMID);
        }
    }
}
