// Fill out your copyright notice in the Description page of Project Settings.

#include "TopDownVision/Public/LineOfSight/VisionComps/Vision_VisualComp.h"

#include "LineOfSight/LOSVisual/LOSStampDrawerComp.h"
#include "LineOfSight/WorldObstacle/LOSObstacleDrawerComponent.h"
#include "LineOfSight/WorldObstacle/LocalTextureSampler.h"
#include "LineOfSight/LOSVisual/VisibilityMeshComp.h"
#include "LineOfSight/ObjectTracing/TopDown2DShapeComp.h"

#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"
#include "LineOfSight/Management/Subsystem/LOSRequirementPoolSubsystem.h"
#include "LineOfSight/VisionComps/Vision_EvaluatorComp.h"
#include "ObstacleOcclusion/Manager/OcclusionSubsystem.h"

#include "GameFramework/GameStateBase.h"
#include "LineOfSight/Management/VisionGameStateComp.h"
#include "TopDownVisionDebug.h"


UVision_VisualComp::UVision_VisualComp()
{
    PrimaryComponentTick.bCanEverTick = false;

    ObstacleDrawer = CreateDefaultSubobject<ULOSObstacleDrawerComponent>(TEXT("ObstacleDrawer"));
    StampDrawer    = CreateDefaultSubobject<ULOSStampDrawerComp>(TEXT("StampDrawer"));
    VisibilityMesh = CreateDefaultSubobject<UVisibilityMeshComp>(TEXT("VisibilityMesh"));
    ShapeComp      = CreateDefaultSubobject<UTopDown2DShapeComp>(TEXT("2DShapeComp"));
}

void UVision_VisualComp::BeginPlay()
{
    Super::BeginPlay();

    if (ShapeComp)
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
                TEXT("[%s] UVision_VisualComp::BeginPlay >> No root component for ShapeComp"),
                *GetOwner()->GetName());
        }
    }

    if (!ShouldRunClientLogic())
        return;
}

void UVision_VisualComp::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    if (GetWorld())
        GetWorld()->GetTimerManager().ClearTimer(FadeTimerHandle);

    if (bUseResourcePool && bHasActivePoolSlot)
    {
        if (ULOSRequirementPoolSubsystem* PoolSub = GetWorld()->GetSubsystem<ULOSRequirementPoolSubsystem>())
            PoolSub->ReleaseSlot(this);
    }

    if (ULOSVisionSubsystem* Subsystem = GetWorld()->GetSubsystem<ULOSVisionSubsystem>())
        Subsystem->UnregisterProvider(this, VisionChannel);

    if (OcclusionTargetIndex != INDEX_NONE)
    {
        if (UOcclusionSubsystem* OccSub = GetWorld()->GetSubsystem<UOcclusionSubsystem>())
        {
            if (UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()))
                OccSub->UnregisterTarget(Root);
        }
        OcclusionTargetIndex = INDEX_NONE;
    }
}

void UVision_VisualComp::OnRegister()
{
    Super::OnRegister();
}

// -------------------------------------------------------------------------- //
//  Initialize
// -------------------------------------------------------------------------- //

void UVision_VisualComp::Initialize()
{
    if (!ShouldRunClientLogic())
        return;

    if (!IsSharedVisionChannel() && IndicatorRange > 0.f)
    {
        VisionRange    = IndicatorRange;
        MaxVisionRange = IndicatorRange;

        UE_LOG(LOSVision, Log,
            TEXT("[%s] Initialize >> IndicatorRange override: %.1f"),
            *GetOwner()->GetName(), IndicatorRange);
    }

    if (bUseResourcePool)
    {
        // ObstacleRT and StampMID deferred to OnPoolSlotAcquired
        if (ObstacleDrawer)
            ObstacleDrawer->InitializeSamplerOnly(MaxVisionRange);

        // VisibilityMesh always locally owned — initialize now
        // SetMeshKey() should be called after monster data loads to re-run FindMeshesByTag
        if (VisibilityMesh)
            VisibilityMesh->Initialize();

        UE_LOG(LOSVision, Log,
            TEXT("[%s] Initialize >> Pool mode — ObstacleRT and StampMID deferred"),
            *GetOwner()->GetName());
    }
    else
    {
        if (ObstacleDrawer)
            ObstacleDrawer->Initialize(MaxVisionRange);

        if (StampDrawer)
        {
            StampDrawer->CreateResources();
            StampDrawer->OnVisionRangeChanged(VisionRange, MaxVisionRange);
        }

        if (VisibilityMesh)
            VisibilityMesh->Initialize();

        UE_LOG(LOSVision, Log,
            TEXT("[%s] Initialize >> Owned mode — all resources created"),
            *GetOwner()->GetName());
    }

    CachedEvaluatorComp = GetOwner()->FindComponentByClass<UVision_EvaluatorComp>();

    if (ULOSVisionSubsystem* Subsystem = GetWorld()->GetSubsystem<ULOSVisionSubsystem>())
        Subsystem->RegisterProvider(this, VisionChannel);
    else
        UE_LOG(LOSVision, Warning,
            TEXT("[%s] Initialize >> LOSVisionSubsystem not found"), *GetOwner()->GetName());

    if (UOcclusionSubsystem* OccSub = GetWorld()->GetSubsystem<UOcclusionSubsystem>())
    {
        if (UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()))
            OcclusionTargetIndex = OccSub->RegisterTarget(Root, nullptr, VisionRange);
    }
}

void UVision_VisualComp::SetIndicatorRange(float NewIndicatorRange)
{
    IndicatorRange = NewIndicatorRange;
}

// -------------------------------------------------------------------------- //
//  Pool enter / exit
// -------------------------------------------------------------------------- //

void UVision_VisualComp::OnRevealed_EnterPool()
{
    if (!bUseResourcePool || bHasActivePoolSlot)
        return;

    if (ULOSRequirementPoolSubsystem* PoolSub = GetWorld()->GetSubsystem<ULOSRequirementPoolSubsystem>())
        PoolSub->AcquireSlot(this);
}

void UVision_VisualComp::OnHidden_ExitPool()
{
    if (!bUseResourcePool || !bHasActivePoolSlot)
        return;

    if (ULOSRequirementPoolSubsystem* PoolSub = GetWorld()->GetSubsystem<ULOSRequirementPoolSubsystem>())
        PoolSub->ReleaseSlot(this);
}

void UVision_VisualComp::OnPoolSlotAcquired(const FLOSStampPoolSlot& Slot)
{
    // ObstacleRT → sampler
    if (ObstacleDrawer)
        if (ULocalTextureSampler* Sampler = ObstacleDrawer->GetLocalTextureSampler())
            Sampler->SetLocalRenderTargetOnly(Slot.ObstacleRT);

    // StampMID → stamp drawer
    if (StampDrawer)
    {
        StampDrawer->SetStampMID(Slot.StampMID);
        StampDrawer->OnVisionRangeChanged(VisionRange, MaxVisionRange);
    }

    // VisibilityMesh — always locally owned, never touched by pool

    bHasActivePoolSlot = true;

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] OnPoolSlotAcquired >> ObstacleRT and StampMID bound"),
        *GetOwner()->GetName());
}

void UVision_VisualComp::OnPoolSlotReleased()
{
    if (ObstacleDrawer)
        if (ULocalTextureSampler* Sampler = ObstacleDrawer->GetLocalTextureSampler())
            Sampler->SetLocalRenderTargetOnly(nullptr);

    if (StampDrawer)
        StampDrawer->SetStampMID(nullptr);

    // VisibilityMesh — left alone, locally owned

    bHasActivePoolSlot = false;

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] OnPoolSlotReleased >> ObstacleRT and StampMID unbound"),
        *GetOwner()->GetName());
}

// -------------------------------------------------------------------------- //
//  Update
// -------------------------------------------------------------------------- //

void UVision_VisualComp::UpdateVision()
{
    if (!ShouldRunClientLogic())
        return;

    if (bUseResourcePool && !bHasActivePoolSlot)
        return;

    if (ObstacleDrawer)
    {
        const FVector CurrentLocation = GetOwner()->GetActorLocation();
        const float MoveDelta = FVector::Dist2D(CurrentLocation, LastObstacleDrawLocation);

        if (MoveDelta >= ObstacleRedrawThreshold)
        {
            LastObstacleDrawLocation = CurrentLocation;
            ObstacleDrawer->UpdateObstacleTexture();
        }
    }

    if (StampDrawer)
    {
        StampDrawer->SetVisionAlpha(VisibilityAlpha);
        StampDrawer->UpdateLOSStamp(ObstacleDrawer ? ObstacleDrawer->GetObstacleRenderTarget() : nullptr);
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

    if (bVisible)
        OnTargetRevealed.Broadcast();
    else
        OnTargetHidden.Broadcast();

    if (bInstant)
    {
        if (VisibilityMesh)
            VisibilityMesh->UpdateVisibility(bVisible);
        return;
    }

    if (!GetWorld()->GetTimerManager().IsTimerActive(FadeTimerHandle))
    {
        GetWorld()->GetTimerManager().SetTimer(
            FadeTimerHandle,
            FTimerDelegate::CreateUObject(this, &UVision_VisualComp::UpdateVisibilityFade, FadeTickInterval),
            FadeTickInterval, true);
    }
}

void UVision_VisualComp::UpdateVisibilityFade(float DeltaTime)
{
    if (!IsValid(this) || !IsValid(GetOwner()) || !GetWorld())
    {
        if (GetWorld())
            GetWorld()->GetTimerManager().ClearTimer(FadeTimerHandle);
        return;
    }

    VisibilityAlpha = FMath::FInterpTo(VisibilityAlpha, TargetVisibilityAlpha, DeltaTime, FadeSpeed);

    if (VisibilityMesh)
        VisibilityMesh->UpdateVisibility(VisibilityAlpha);

    if (OcclusionTargetIndex != INDEX_NONE)
    {
        if (UOcclusionSubsystem* OccSub = GetWorld()->GetSubsystem<UOcclusionSubsystem>())
            OccSub->UpdateTargetByIndex(OcclusionTargetIndex, VisibilityAlpha, -1.f);
    }

    if (FMath::IsNearlyEqual(VisibilityAlpha, TargetVisibilityAlpha, 0.005f))
    {
        VisibilityAlpha = TargetVisibilityAlpha;

        if (OcclusionTargetIndex != INDEX_NONE)
        {
            if (UOcclusionSubsystem* OccSub = GetWorld()->GetSubsystem<UOcclusionSubsystem>())
                OccSub->UpdateTargetByIndex(OcclusionTargetIndex, VisibilityAlpha, -1.f);
        }

        GetWorld()->GetTimerManager().ClearTimer(FadeTimerHandle);

        if (VisibilityAlpha > 0)
            OnTargetRevealComplete.Broadcast();
        else
            OnTargetHideComplete.Broadcast();

        UE_LOG(LOSVision, Verbose,
            TEXT("[%s] UpdateVisibilityFade >> Complete: %.2f"),
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
//  Vision channel
// -------------------------------------------------------------------------- //

void UVision_VisualComp::SetVisionChannel(EVisionChannel InVC)
{
    FString TempDebug = TopDownVisionDebug::GetClientDebugName(GetOwner());
    VisionChannel = InVC;
}

void UVision_VisualComp::UpdateVisionRange(float NewRange)
{
    VisionRange = FMath::Clamp(NewRange, 0.f, MaxVisionRange);
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
    if (!GSComp) return EVisionChannel::None;

    return GSComp->GetLocalPlayerTeamChannel();
}

bool UVision_VisualComp::ShouldRunClientLogic() const
{
    return GetNetMode() != NM_DedicatedServer;
}
