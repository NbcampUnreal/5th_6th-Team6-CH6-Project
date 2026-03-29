// Fill out your copyright notice in the Description page of Project Settings.

#include "LineOfSight/WorldObstacle/LOSObstacleDrawerComponent.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

#include "LineOfSight/WorldObstacle/LocalTextureSampler.h"
#include "EditorSetting/LOSResourcePoolSettings.h"

#include "TopDownVisionDebug.h"



ULOSObstacleDrawerComponent::ULOSObstacleDrawerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    LocalTextureSampler = CreateDefaultSubobject<ULocalTextureSampler>(TEXT("LocalTextureSampler"));
}

void ULOSObstacleDrawerComponent::BeginPlay()
{
    Super::BeginPlay();
}

// -------------------------------------------------------------------------- //
//  Initialize — owned mode
// -------------------------------------------------------------------------- //

void ULOSObstacleDrawerComponent::Initialize(float InMaxVisionRange)
{
    MaxVisionRange = FMath::Max(1.f, InMaxVisionRange);
    CreateResources();

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] ULOSObstacleDrawerComponent::Initialize >> Owned mode | MaxVisionRange=%.1f"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()), MaxVisionRange);
}

// -------------------------------------------------------------------------- //
//  InitializeSamplerOnly — pool mode
// -------------------------------------------------------------------------- //

void ULOSObstacleDrawerComponent::InitializeSamplerOnly(float InMaxVisionRange)
{
    MaxVisionRange = FMath::Max(1.f, InMaxVisionRange);

    // Wire sampler but leave RT null — pool subsystem assigns it on slot acquire
    SetupSampler();

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] ULOSObstacleDrawerComponent::InitializeSamplerOnly >> Pool mode | MaxVisionRange=%.1f — RT deferred"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()), MaxVisionRange);
}

// -------------------------------------------------------------------------- //
//  Update
// -------------------------------------------------------------------------- //

void ULOSObstacleDrawerComponent::UpdateObstacleTexture()
{
    if (!LocalTextureSampler)
    {
        UE_LOG(LOSVision, Error,
            TEXT("[%s] ULOSObstacleDrawerComponent::UpdateObstacleTexture >> LocalTextureSampler missing"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    // Works for both modes — in pool mode the sampler's RT is assigned by the subsystem,
    // in owned mode it was set during CreateResources. Either way the sampler knows its RT.
    if (!LocalTextureSampler->GetLocalRenderTarget())
    {
        UE_LOG(LOSVision, Verbose,
            TEXT("[%s] ULOSObstacleDrawerComponent::UpdateObstacleTexture >> No RT on sampler yet — skipping"),
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
            FQuat::Identity, FColor::Green,
            false, -1.f, 0, 2.f);
    }

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] ULOSObstacleDrawerComponent::UpdateObstacleTexture >> Sampled"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()));
}

// -------------------------------------------------------------------------- //
//  Getter
// -------------------------------------------------------------------------- //

UTextureRenderTarget2D* ULOSObstacleDrawerComponent::GetObstacleRenderTarget() const
{
    // Pool mode — RT lives on the sampler
    if (LocalTextureSampler && LocalTextureSampler->GetLocalRenderTarget())
        return LocalTextureSampler->GetLocalRenderTarget();

    // Owned mode — RT lives here
    return ObstacleRenderTarget;
}

// -------------------------------------------------------------------------- //
//  CreateResources — owned mode only
// -------------------------------------------------------------------------- //

void ULOSObstacleDrawerComponent::CreateResources()
{
    UWorld* World = GetWorld();
    if (!World)
        return;

    // ---- Render Target (owned) ---- //
    if (!ObstacleRenderTarget)
    {
        const ULOSResourcePoolSettings* Settings = ULOSResourcePoolSettings::Get();

        UTextureRenderTarget2D* Template = Settings
            ? Settings->TemplateObstacleRT.LoadSynchronous()
            : nullptr;

        FString RTName = FString::Printf(TEXT("ObstacleRT_%s"), *GetOwner()->GetName());
        ObstacleRenderTarget = NewObject<UTextureRenderTarget2D>(this, FName(*RTName));

        if (Template)
        {
            ObstacleRenderTarget->RenderTargetFormat = Template->RenderTargetFormat;
            ObstacleRenderTarget->ClearColor         = Template->ClearColor;
            ObstacleRenderTarget->bAutoGenerateMips  = Template->bAutoGenerateMips;
            ObstacleRenderTarget->InitAutoFormat(Template->SizeX, Template->SizeY);
        }
        else
        {
            // Fallback if settings not configured — keeps owned mode working without pool setup
            UE_LOG(LOSVision, Warning,
                TEXT("[%s] ULOSObstacleDrawerComponent::CreateResources >> TemplateObstacleRT not set — using fallback 256 RGBA8"),
                *TopDownVisionDebug::GetClientDebugName(GetOwner()));

            ObstacleRenderTarget->RenderTargetFormat = RTF_RGBA8;
            ObstacleRenderTarget->ClearColor         = FLinearColor::Black;
            ObstacleRenderTarget->InitAutoFormat(256, 256);
        }

        ObstacleRenderTarget->UpdateResource();

        UE_LOG(LOSVision, Verbose,
            TEXT("[%s] ULOSObstacleDrawerComponent::CreateResources >> RT created: %s (%p)"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()),
            *ObstacleRenderTarget->GetName(), ObstacleRenderTarget);
    }

    // ---- Sampler ---- //
    SetupSampler();

    // Pass RT to sampler last — triggers first draw
    if (LocalTextureSampler)
        LocalTextureSampler->SetLocalRenderTarget(ObstacleRenderTarget);
}

// -------------------------------------------------------------------------- //
//  SetupSampler — shared between both init paths
// -------------------------------------------------------------------------- //

void ULOSObstacleDrawerComponent::SetupSampler()
{
    if (!LocalTextureSampler)
        return;

    UWorld* World = GetWorld();
    LocalTextureSampler->SetCachedWorld(World);

    AActor* OwningActor = GetOwner();
    if (!OwningActor)
    {
        UE_LOG(LOSVision, Error,
            TEXT("[%s] ULOSObstacleDrawerComponent::SetupSampler >> No owning actor"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    if (USceneComponent* Root = OwningActor->GetRootComponent())
    {
        LocalTextureSampler->SetLocationRoot(Root);

        UE_LOG(LOSVision, Verbose,
            TEXT("[%s] ULOSObstacleDrawerComponent::SetupSampler >> Root: %s"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()), *Root->GetName());
    }
    else
    {
        UE_LOG(LOSVision, Error,
            TEXT("[%s] ULOSObstacleDrawerComponent::SetupSampler >> Could not find valid root"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
    }

    LocalTextureSampler->SetWorldSampleRadius(MaxVisionRange);
}
