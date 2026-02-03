// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/LineOfSightComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TopDownVisionLogCategories.h"// log

//environment texture source
#include "Components/SceneCaptureComponent2D.h"
#include "LineOfSight/WorldObstacle/LocalTextureSampler.h"

#include "DrawDebugHelpers.h"//debug for visualizing the activation
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"


ULineOfSightComponent::ULineOfSightComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // Manual draw

    //Create 2DSceneCaptureComp
    SceneCaptureComp = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("LOS_SceneCapture"));
    SceneCaptureComp->SetupAttachment(this);//attach to the owner's root

    //Basic Settings
    SceneCaptureComp->ProjectionType=ECameraProjectionMode::Orthographic;//set to orthographic
    SceneCaptureComp->OrthoWidth=MaxVisionRange*2;//width is double of MaxVisible range

    SceneCaptureComp->CaptureSource=SCS_SceneDepth;//--> dont use Device depth, use SceneDepth which has actual distacne
    //Should the depth be device depth or Scene depth? fuck
    
    //activation setting -> only captured when it is called
    SceneCaptureComp->bCaptureEveryFrame=false;
    SceneCaptureComp->bCaptureOnMovement=false;
    
    //Show only setting -> can register a list of actors to be not rendered
    //SceneCaptureComp->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    //-> TODO: this will be used when the Tag System is ready, but for now, just use Legacy Render
    
    SceneCaptureComp->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_LegacySceneCapture;
   // SceneCaptureComp->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
    //Transform Setting -> rotation need to be absolute so that it does not rotate with the owner
    SceneCaptureComp->SetUsingAbsoluteRotation(true);
    SceneCaptureComp->SetWorldRotation(FRotator(0, -90, 0));
}

void ULineOfSightComponent::BeginPlay()
{
    Super::BeginPlay();

    CreateResources();// make RT and MID

    RefreshObstaclesByTag();// find and register the obstacles
}

void ULineOfSightComponent::UpdateLocalLOS()
{
    if (!ShouldUpdate)
    {
        /*UE_LOG(LOSVision, Log,
            TEXT("ULineOfSightComponent::UpdateLocalLOS >> Skipped, ShouldUpdate is false"));*/
        return;
    }
    if (!LOSRenderTarget)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("ULineOfSightComponent::UpdateLocalLOS >> Invalid HeightRenderTarget"));
        return;
    }

    if (bUseSceneCapture)// scene capture mode
    {
        // SceneCapture mode
        if (!SceneCaptureComp)
        {
            UE_LOG(LOSVision, Error,
                TEXT("ULineOfSightComponent::UpdateLocalLOS >> Invalid SceneCaptureComp"));
            return;
        }

        SceneCaptureComp->CaptureScene();
    }
    else //local sampler mode
    {
        if (!LocalTextureSampler)
        {
            UE_LOG(LOSVision, Error,
                TEXT("ULineOfSightComponent::UpdateLocalLOS >> LocalTextureSampler missing"));
            return;
        }

        LocalTextureSampler->UpdateLocalTexture();
    }

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
    
    /*UE_LOG(LOSVision, Log,
        TEXT("ULineOfSightComponent::UpdateLocalLOS >> UpdateResource called"));*/
}

void ULineOfSightComponent::UpdateVisibleRange(float NewRange)
{
    VisionRange = FMath::Max(0.f, NewRange); // clamp to non-negative

    /*UE_LOG(LOSVision, Log,
        TEXT("ULineOfSightComponent::UpdateVisibleRange >> VisionRange set to %f"),
        VisionRange);*/
}


void ULineOfSightComponent::ToggleUpdate(bool bIsOn)
{
    if (ShouldUpdate==bIsOn)
    {
        /*UE_LOG(LOSVision, Log,
            TEXT("ULineOfSightComponent::ToggleUpdate >> Already set to %s"),
            ShouldUpdate ? TEXT("true") : TEXT("false"));*/
        return;
    }
    
    ShouldUpdate = bIsOn;

    /*
    UE_LOG(LOSVision, Log,
        TEXT("ULineOfSightComponent::ToggleUpdate >> ShouldUpdate set to %s"),
        ShouldUpdate ? TEXT("true") : TEXT("false"));*/
}

void ULineOfSightComponent::RegisterObstacle(AActor* Obstacle)
{
    if (SceneCaptureComp && Obstacle)
    {
        // adds the actor's components to the "Show Only" whitelist
        SceneCaptureComp->ShowOnlyActorComponents(Obstacle);
    }
}



void ULineOfSightComponent::CreateResources()
{
    if (!GetWorld())
        return;

    //make RT
    if (!LOSRenderTarget)
    {
        LOSRenderTarget = NewObject<UTextureRenderTarget2D>(this);
        LOSRenderTarget->InitAutoFormat(PixelResolution, PixelResolution);
        LOSRenderTarget->ClearColor = FLinearColor::Black;
    }

    if (bUseSceneCapture)
    {
        if (!SceneCaptureComp)
        {
            UE_LOG(LOSVision, Error,
                TEXT("ULineOfSightComponent::CreateResources >> SceneCaptureComp missing"));
            return;
        }

        LOSRenderTarget->RenderTargetFormat = RTF_R16f;
        SceneCaptureComp->TextureTarget = LOSRenderTarget;
    }
    else // LocalTextureSampler Mode
    {
        if (!LocalTextureSampler)
        {
            LocalTextureSampler = NewObject<ULocalTextureSampler>(
                this,
                ULocalTextureSampler::StaticClass(),
                TEXT("LOS_LocalSampler"));
            
            if (!LocalTextureSampler)
            {
                UE_LOG(LOSVision, Error,
                    TEXT("ULineOfSightComponent::CreateResources >> Failed to create LocalTextureSampler"));
                return;
            }

            LocalTextureSampler->RegisterComponent();
            LocalTextureSampler->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
        }

        LOSRenderTarget->RenderTargetFormat = RTF_R8; // only R/G needed
        LocalTextureSampler->SetLocalRenderTarget(LOSRenderTarget);
    }

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


