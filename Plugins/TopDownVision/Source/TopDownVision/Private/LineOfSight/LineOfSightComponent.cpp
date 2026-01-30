// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/LineOfSightComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TopDownVisionLogCategories.h"// log
#include "Components/SceneCaptureComponent2D.h"
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

    CreateResources();// make CRT and MID

    RefreshObstaclesByTag();// find and register the obstacles
}

void ULineOfSightComponent::UpdateLocalLOS()
{
    if (!ShouldUpdate)
    {
        UE_LOG(LOSVision, Log,
            TEXT("ULineOfSightComponent::UpdateLocalLOS >> Skipped, ShouldUpdate is false"));
        return;
    }

    if (!SceneCaptureComp)
    {
        UE_LOG(LOSVision, Error,
            TEXT("ULineOfSightComponent::UpdateLocalLOS >> Invalid SceneCaptureComp"));
        return;
    }
    
    if (!HeightRenderTarget)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("ULineOfSightComponent::UpdateLocalLOS >> Invalid HeightRenderTarget"));
        return;
    }
    
    SceneCaptureComp->CaptureScene();// Capture the scene

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
    
    UE_LOG(LOSVision, Log,
        TEXT("ULineOfSightComponent::UpdateLocalLOS >> UpdateResource called"));
}

void ULineOfSightComponent::UpdateVisibleRange(float NewRange)
{
    VisionRange = FMath::Max(0.f, NewRange); // clamp to non-negative

    UE_LOG(LOSVision, Log,
        TEXT("ULineOfSightComponent::UpdateVisibleRange >> VisionRange set to %f"),
        VisionRange);
}


void ULineOfSightComponent::ToggleUpdate(bool bIsOn)
{
    if (ShouldUpdate==bIsOn)
    {
        UE_LOG(LOSVision, Log,
            TEXT("ULineOfSightComponent::ToggleUpdate >> Already set to %s"),
            ShouldUpdate ? TEXT("true") : TEXT("false"));
        return;
    }
    
    ShouldUpdate = bIsOn;

    UE_LOG(LOSVision, Log,
        TEXT("ULineOfSightComponent::ToggleUpdate >> ShouldUpdate set to %s"),
        ShouldUpdate ? TEXT("true") : TEXT("false"));
}

void ULineOfSightComponent::RegisterObstacle(AActor* Obstacle)
{
    if (SceneCaptureComp && Obstacle)
    {
        // adds the actor's components to the "Show Only" whitelist
        SceneCaptureComp->ShowOnlyActorComponents(Obstacle);
    }
}

void ULineOfSightComponent::RefreshObstaclesByTag()//should this be in the subsystem?
{
    if (!GetWorld() || BlockerTag.IsNone()) return;

    //Catchers
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsWithTag(
        GetWorld(),
        BlockerTag,
        FoundActors);

    for (AActor* Actor : FoundActors)
    {
        RegisterObstacle(Actor);
    }

    UE_LOG(LOSVision, Log,
        TEXT("ULineOfSightComponent::RefreshObstaclesByTag >> Registered %d obstacles with tag: %s"),
        FoundActors.Num(), *BlockerTag.ToString());
}


void ULineOfSightComponent::CreateResources()
{
    if (!GetWorld() || !SceneCaptureComp)
        return;

    HeightRenderTarget = NewObject<UTextureRenderTarget2D>(this);
    if (!HeightRenderTarget)
    {
        UE_LOG(LOSVision, Error,
            TEXT("Failed to allocate HeightRenderTarget"));
        return;
    }

    //HeightRenderTarget->RenderTargetFormat = RTF_R32f;
    HeightRenderTarget->RenderTargetFormat = RTF_R16f;
    //HeightRenderTarget->RenderTargetFormat = RTF_R8;
    
    HeightRenderTarget->InitAutoFormat(PixelResolution, PixelResolution);
    HeightRenderTarget->ClearColor = FLinearColor::Black;
    //HeightRenderTarget->UpdateResourceImmediate(true);

    SceneCaptureComp->TextureTarget = HeightRenderTarget;

    UE_LOG(LOSVision, Log,
        TEXT("HeightRenderTarget created (%dx%d)"),
        PixelResolution, PixelResolution);

    if (!LOSMaterial)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("LOSMaterial is null"));
        return;
    }

    LOSMaterialMID = UMaterialInstanceDynamic::Create(LOSMaterial, this);
    if (!LOSMaterialMID)
    {
        UE_LOG(LOSVision, Error,
            TEXT("Failed to create LOSMaterialMID"));
        return;
    }

    LOSMaterialMID->SetTextureParameterValue(// set Texture Param
        MIDTextureParam,
        HeightRenderTarget);

    LOSMaterialMID->SetScalarParameterValue(//Set Vision Radius Param
        MIDVisibleRangeParam,
        VisionRange/MaxVisionRange/2);// normalized/2 for half radius
    
    LOSMaterialMID->SetScalarParameterValue(//Set EyeSight Param
        MIDEyeSightHeightParam,
        EyeSightHeight);
    

    UE_LOG(LOSVision, Log,
        TEXT("LOSMaterialMID initialized"));
}


