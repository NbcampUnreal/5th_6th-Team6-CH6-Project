// Fill out your copyright notice in the Description page of Project Settings.

#include "LineOfSight/WorldObstacle/LOSObstacleDrawerComponent.h"

#include "Engine/TextureRenderTarget2D.h"
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
    // CreateResources is called via Initialize() from Vision_VisualComp::BeginPlay
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
    UWorld* World = GetWorld();
    if (!World)
        return;

    // ---- Render Target ---- //
    if (!ObstacleRenderTarget)
    {
        FString RTName = FString::Printf(TEXT("ObstacleRT_%s"), *GetOwner()->GetName());
        ObstacleRenderTarget = NewObject<UTextureRenderTarget2D>(this, FName(*RTName));
        ObstacleRenderTarget->InitAutoFormat(PixelResolution, PixelResolution);
        ObstacleRenderTarget->ClearColor        = FLinearColor::Black;
        //ObstacleRenderTarget->RenderTargetFormat = RTF_R8;
        ObstacleRenderTarget->RenderTargetFormat = RTF_RGBA8;
        ObstacleRenderTarget->UpdateResourceImmediate();

        UE_LOG(LOSVision, Log,
            TEXT("[%s] ULOSObstacleDrawerComponent::CreateResources >> RT created: %s (%p)"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()),
            *ObstacleRenderTarget->GetName(), ObstacleRenderTarget);
    }

    // ---- Local Texture Sampler ---- //
    if (LocalTextureSampler)
    {
        // Inject world explicitly — GetWorld() inside LocalTextureSampler
        // may fail when nested inside another component's subobject chain
        LocalTextureSampler->SetCachedWorld(World);

        // GetOwner() on ActorComponent always returns the actor directly
        // regardless of how the component was constructed
        USceneComponent* BestRoot = nullptr;
        AActor* OwningActor = GetOwner();
        if (OwningActor)
        {
            BestRoot = OwningActor->GetRootComponent();
            UE_LOG(LOSVision, Log,
                TEXT("[%s] ULOSObstacleDrawerComponent::CreateResources >> Root resolved: %s"),
                *TopDownVisionDebug::GetClientDebugName(GetOwner()),
                *BestRoot->GetName());
        }

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
        LocalTextureSampler->SetLocalRenderTarget(ObstacleRenderTarget); // last — triggers first draw
    }
}
