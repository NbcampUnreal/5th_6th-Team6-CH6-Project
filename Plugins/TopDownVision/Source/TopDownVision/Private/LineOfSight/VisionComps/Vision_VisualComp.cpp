// Fill out your copyright notice in the Description page of Project Settings.

//Class
#include "TopDownVision/Public/LineOfSight/VisionComps/Vision_VisualComp.h"

//comps
#include "LineOfSight/LOSVisual/LOSStampDrawerComp.h"
#include "LineOfSight/WorldObstacle/LOSObstacleDrawerComponent.h"
#include "LineOfSight/LOSVisual/VisibilityMeshComp.h"
#include "LineOfSight/ObjectTracing/TopDown2DShapeComp.h"

//Debug
#include "TopDownVisionDebug.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "LineOfSight/Management/VisionGameStateComp.h"
#include "LineOfSight/Management/VisionPlayerStateComp.h"
#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"
#include "LineOfSight/VisionComps/Vision_EvaluatorComp.h"


UVision_VisualComp::UVision_VisualComp()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    ObstacleDrawer = CreateDefaultSubobject<ULOSObstacleDrawerComponent>(TEXT("ObstacleDrawer"));
    StampDrawer =    CreateDefaultSubobject<ULOSStampDrawerComp>(TEXT("StampDrawer"));
    VisibilityMesh = CreateDefaultSubobject<UVisibilityMeshComp>(TEXT("VisibilityMesh"));
    ShapeComp =      CreateDefaultSubobject<UTopDown2DShapeComp>(TEXT("2DShapeComp"));
    //ShapeComp->SetupAttachment(GetOwner()->GetRootComponent());-> do this on runtime, not right now
}

void UVision_VisualComp::BeginPlay()
{
    Super::BeginPlay();
    
    if (ShapeComp)//attachment happens here
    {
        if (USceneComponent* Root = GetOwner()->GetRootComponent())
        {
            ShapeComp->AttachToComponent(
                Root,
                FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        }
        else
        {
            UE_LOG(LOSVision, Warning,
                TEXT("[%s] UVision_VisualComp::Initialize >> Owner has no root component, cannot attach ShapeComp"),
                *GetOwner()->GetName());
        }
    }
    else
    {
        UE_LOG(LOSVision, Error,
            TEXT("[%s] UVision_VisualComp::Initialize >> ShapeComp subobject is missing"),
            *GetOwner()->GetName());
    }

    
    if (!ShouldRunClientLogic())//server gate
        return;

    //Initialize();
}

void UVision_VisualComp::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    //Clean up!
    if (ULOSVisionSubsystem* Subsystem = GetWorld()->GetSubsystem<ULOSVisionSubsystem>())
        Subsystem->UnregisterProvider(this, VisionChannel);
}

void UVision_VisualComp::OnRegister()
{
    Super::OnRegister();

 
    
}

void UVision_VisualComp::Initialize()
{
    
    AActor* DebugCheckingActor = GetOwner();

    if (!ShouldRunClientLogic())
        return;

    // Must happen before ObstacleDrawer and StampDrawer initialize
    // so RT size and UV params are built at the correct range from the start
    if (!IsSharedVisionChannel() && IndicatorRange > 0.f)
    {
        VisionRange    = IndicatorRange;
        MaxVisionRange = IndicatorRange;

        UE_LOG(LOSVision, Log,
            TEXT("[%s] Initialize >> Not shared channel, overriding ranges to IndicatorRange: %.1f"),
            *GetOwner()->GetName(), IndicatorRange);
    }

    // Now builds RT and UV at whatever range was set above
    if (ObstacleDrawer)
        ObstacleDrawer->Initialize(MaxVisionRange);

    if (StampDrawer)
    {
        StampDrawer->CreateResources();
        StampDrawer->OnVisionRangeChanged(VisionRange, MaxVisionRange);
    }

    if (VisibilityMesh)
        VisibilityMesh->Initialize();

    CachedEvaluatorComp = GetOwner()->FindComponentByClass<UVision_EvaluatorComp>();

    if (ULOSVisionSubsystem* Subsystem = GetWorld()->GetSubsystem<ULOSVisionSubsystem>())
    {
        Subsystem->RegisterProvider(this, VisionChannel);
    }
    else
    {
        UE_LOG(LOSVision, Warning,
            TEXT("[%s] Initialize >> LOSVisionSubsystem not found"),
            *GetOwner()->GetName());
    }
}

void UVision_VisualComp::SetIndicatorRange(float NewIndicatorRange)
{

    IndicatorRange =  NewIndicatorRange;

}

// -------------------------------------------------------------------------- //
//  Update
// -------------------------------------------------------------------------- //

void UVision_VisualComp::UpdateVision()
{
    if (!ShouldRunClientLogic())
        return;

    if (ObstacleDrawer)
        ObstacleDrawer->UpdateObstacleTexture();

    if (StampDrawer)
    {
        StampDrawer->SetVisionAlpha(VisibilityAlpha);
        StampDrawer->UpdateLOSStamp( ObstacleDrawer ? ObstacleDrawer->GetObstacleRenderTarget() : nullptr);
    }
       
}

void UVision_VisualComp::ToggleLOSStampUpdate(bool bIsOn)
{
    if (StampDrawer)
        StampDrawer->ToggleLOSStampUpdate(bIsOn);
}

bool UVision_VisualComp::IsUpdating() const
{
    return StampDrawer ? StampDrawer->IsUpdating() : false;
}

// -------------------------------------------------------------------------- //
//  Visibility Fade
// -------------------------------------------------------------------------- //

void UVision_VisualComp::SetVisible(bool bVisible, bool bInstant)
{
    TargetVisibilityAlpha = bVisible ? 1.0f : 0.0f;

    if (bInstant)//update instantly
    {
        if (VisibilityMesh)
            VisibilityMesh->UpdateVisibility(bVisible);
        return;
    }
    
    if (!GetWorld()->GetTimerManager().IsTimerActive(FadeTimerHandle))
    {
        GetWorld()->GetTimerManager().SetTimer(
            FadeTimerHandle, this,
            &UVision_VisualComp::UpdateVisibilityFade,
            0.016f, true);
    }
}

void UVision_VisualComp::UpdateVisibilityFade()
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

void UVision_VisualComp::SetVisionRange(float NewRange)
{
    VisionRange = FMath::Clamp(NewRange, 0.f, MaxVisionRange);

    if (StampDrawer)
        StampDrawer->OnVisionRangeChanged(VisionRange, MaxVisionRange);

    // Lazy cache — null is valid for actors without an evaluator
    if (!CachedEvaluatorComp)
        CachedEvaluatorComp = GetOwner()->FindComponentByClass<UVision_EvaluatorComp>();

    if (CachedEvaluatorComp)
        CachedEvaluatorComp->SyncDetectionRadius();
}

UMaterialInstanceDynamic* UVision_VisualComp::GetStampMID() const
{
    return StampDrawer ? StampDrawer->GetLOSMaterialMID() : nullptr;
}

// -------------------------------------------------------------------------- //

void UVision_VisualComp::SetVisionChannel(EVisionChannel InVC)
{
    FString TempDebug = TopDownVisionDebug::GetClientDebugName(GetOwner());
    VisionChannel = InVC;
}

void UVision_VisualComp::UpdateVisionRange(float NewRange)
{
    VisionRange= FMath::Clamp(NewRange, 0.f, MaxVisionRange);
}

bool UVision_VisualComp::IsSharedVisionChannel() const
{
    return VisionChannel == GetLocalPlayerVisionChannel();
}

EVisionChannel UVision_VisualComp::GetLocalPlayerVisionChannel() const
{
    AGameStateBase* GS = GetWorld()->GetGameState();
    if (!GS) return EVisionChannel::None;

    UVisionGameStateComp* GSComp = GS->FindComponentByClass<UVisionGameStateComp>();
    if (!GSComp) return  EVisionChannel::None;

    return GSComp->GetLocalPlayerTeamChannel();
}

bool UVision_VisualComp::ShouldRunClientLogic() const
{
    return GetNetMode() != NM_DedicatedServer;
}
