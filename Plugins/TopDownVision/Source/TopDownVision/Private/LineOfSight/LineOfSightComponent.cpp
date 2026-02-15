// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/LineOfSightComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TopDownVisionDebug.h"// log

//environment texture source
#include "LineOfSight/WorldObstacle/LocalTextureSampler.h"

// Vision helpers
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "LineOfSight/ObjectTracing/ShapeAwareVisibilityTracer.h"

#include "DrawDebugHelpers.h"//debug for visualizing the activation
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "LineOfSight/Management/VisionGameStateComp.h"
#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"


ULineOfSightComponent::ULineOfSightComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // Manual draw
    SetIsReplicatedByDefault(true);//replication on

    //Local Texture Sampler
    LocalTextureSampler = CreateDefaultSubobject<ULocalTextureSampler>(TEXT("LocalTextureSampler"));


    //=== Visible Target Detection ===//
    VisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("VisionSphere"));
    //VisionSphere->SetupAttachment(GetOwner()->GetRootComponent()); --> too early to get the root from owner -> put this in the begin play

    VisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);//make it collide with
    VisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
    VisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    VisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    VisionSphere->SetGenerateOverlapEvents(true);
    
}

void ULineOfSightComponent::BeginPlay()
{
    Super::BeginPlay();

    //Degug
    UE_LOG(LOSVision, Log,
    TEXT("[%s] LOS BeginPlay | Owner=%s | Role=%d | RemoteRole=%d | IsLocallyControlled=%d"),
    *TopDownVisionDebug::GetClientDebugName(GetOwner()),
    *GetOwner()->GetName(),
    (int32)GetOwner()->GetLocalRole(),
    (int32)GetOwner()->GetRemoteRole(),
    GetOwner()->IsOwnedBy(GetWorld()->GetFirstPlayerController()));
    
    if (!ShouldRunClientLogic())
    {
        return;// not for server
    }
    
    CreateResources();// make RT and MID

    //Target Detection
    if (VisionSphere)
    {
       VisionSphere->AttachToComponent(
            GetOwner()->GetRootComponent(),
            FAttachmentTransformRules::KeepRelativeTransform);

        VisionSphere->SetSphereRadius(VisionRange);

        VisionSphere->OnComponentBeginOverlap.AddDynamic(
            this, &ULineOfSightComponent::OnVisionSphereBeginOverlap);

        VisionSphere->OnComponentEndOverlap.AddDynamic(
            this, &ULineOfSightComponent::OnVisionSphereEndOverlap);
    }

    // ===== Create tracer =====
    VisibilityTracer = NewObject<UShapeAwareVisibilityTracer>(this);

    //set attachment of vision sphere
    VisionSphere->SetupAttachment(GetOwner()->GetRootComponent());
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

        if (VisionSphere)//set the radius of the sphere comp
        {
            VisionSphere->SetSphereRadius(VisionRange);
        }

        
        LOSMaterialMID->SetScalarParameterValue(
            MIDVisibleRangeParam, 
            VisionRange / MaxVisionRange / 2.f);
        
        UE_LOG(LOSVision, Verbose,
            TEXT("[%s]ULineOfSightComponent::UpdateVisibleRange >> Updated material: VisionRange=%.1f, Normalized=%.3f"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()),
            VisionRange,
            VisionRange / MaxVisionRange / 2.f);
    }
    
    // Also update LocalTextureSampler's world sample radius
    if (LocalTextureSampler)
    {
        LocalTextureSampler->SetWorldSampleRadius(VisionRange);
    }
}

void ULineOfSightComponent::CreateResources()
{
    UE_LOG(LOSVision, Log,
        TEXT("[%s]ULineOfSightComponent::CreateResources >> Owner=%s"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        *GetOwner()->GetName());
    
    if (!ShouldRunClientLogic())
    {
        return;// not for server
    }
    
    if (!GetWorld())
        return;

    // Create RT with unique name
    if (!LOSRenderTarget)
    {
        FString RTName = FString::Printf(TEXT("LOSRenderTarget_%s"), *GetOwner()->GetName());
        LOSRenderTarget = NewObject<UTextureRenderTarget2D>(this, FName(*RTName));
        LOSRenderTarget->InitAutoFormat(PixelResolution, PixelResolution);
        LOSRenderTarget->ClearColor = FLinearColor::Black;
        LOSRenderTarget->RenderTargetFormat = RTF_R8;

        UE_LOG(LOSVision, Log,
            TEXT("[%s] Created unique RT: %s (Address: %p)"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()),
            *LOSRenderTarget->GetName(),
            LOSRenderTarget);
    }
    
    LOSRenderTarget->RenderTargetFormat = RTF_R8; // only R/G needed
    
    if (!LocalTextureSampler)
    {
        UE_LOG(LOSVision, Error,
            TEXT("[%s]ULineOfSightComponent::CreateResources >> LocalTextureSampler default subobject missing"),
             *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    LocalTextureSampler->SetLocalRenderTarget(LOSRenderTarget);
    LocalTextureSampler->SetLocationRoot(GetOwner()->GetRootComponent());
    LocalTextureSampler->SetWorldSampleRadius(VisionRange);
    // pass the owner's root so that it can know world location

   // Create MID with unique name
    if (LOSMaterial)
    {
        FString MIDName = FString::Printf(TEXT("LOSMID_%s"), *GetOwner()->GetName());
        LOSMaterialMID = UMaterialInstanceDynamic::Create(LOSMaterial, this, FName(*MIDName));
        
        if (LOSMaterialMID && LOSRenderTarget)
        {
            LOSMaterialMID->SetTextureParameterValue(MIDTextureParam, LOSRenderTarget);
            LOSMaterialMID->SetScalarParameterValue(MIDVisibleRangeParam, VisionRange / MaxVisionRange / 2.f);
            
            UE_LOG(LOSVision, Log,
                TEXT("[%s] Created unique MID: %s (Address: %p)"),
                *TopDownVisionDebug::GetClientDebugName(GetOwner()),
                *LOSMaterialMID->GetName(),
                LOSMaterialMID);
        }
    }
    
    UE_LOG(LOSVision, Log,
          TEXT("[%s] ULineOfSightComponent::CreateResources >> | RT=%s (%p) | MID=%s (%p)"),
          *TopDownVisionDebug::GetClientDebugName(GetOwner()),
          LOSRenderTarget ? *LOSRenderTarget->GetName() : TEXT("NULL"),
          LOSRenderTarget,
          LOSMaterialMID ? *LOSMaterialMID->GetName() : TEXT("NULL"),
          LOSMaterialMID);
}

bool ULineOfSightComponent::ShouldRunClientLogic() const
{
    if (GetNetMode() == NM_DedicatedServer)
        return false;
    
    //other conditions

    return true;
}


//======================= LOS Vision Stamp ===========================================================================//
#pragma region VisionStamp Management

void ULineOfSightComponent::UpdateLocalLOS()
{
    UE_LOG(LOSVision, VeryVerbose,
        TEXT("[%s] ULineOfSightComponent::UpdateLocalLOS >> ENTER | Owner=%s | ShouldUpdate=%d"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        *GetOwner()->GetName(),
        ShouldUpdateLOSStamp);
    
    if (!ShouldRunClientLogic())
    {
        return;// not for server
    }

    
    if (!ShouldUpdateLOSStamp)
    {
        UE_LOG(LOSVision, Verbose,
            TEXT("[%s]ULineOfSightComponent::UpdateLocalLOS >> Skipped, ShouldUpdate is false"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }
    if (!LOSRenderTarget)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("[%s]ULineOfSightComponent::UpdateLocalLOS >> Invalid HeightRenderTarget"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }
    // now only use local texture sampler as only a source of RT
    if (!LocalTextureSampler)
    {
        UE_LOG(LOSVision, Error,
            TEXT("[%s]ULineOfSightComponent::UpdateLocalLOS >> LocalTextureSampler missing"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
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
        TEXT("[%s]ULineOfSightComponent::UpdateLocalLOS >> UpdateResource called"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()));
}


void ULineOfSightComponent::ToggleLOSStampUpdate(bool bIsOn)
{
    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] ToggleUpdate | Owner=%s | New=%d"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        *GetOwner()->GetName(),
        bIsOn);
    
    if (ShouldUpdateLOSStamp==bIsOn)
    {
        UE_LOG(LOSVision, Verbose,
            TEXT("[%s]ULineOfSightComponent::ToggleUpdate >> Already set to %s"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()),
            ShouldUpdateLOSStamp ? TEXT("true") : TEXT("false"));
        return;
    }
    
    ShouldUpdateLOSStamp = bIsOn;

    
    UE_LOG(LOSVision, Verbose,
       TEXT("[%s] ToggleUpdate APPLIED | Owner=%s | ShouldUpdate=%d"),
       *TopDownVisionDebug::GetClientDebugName(GetOwner()),
       *GetOwner()->GetName(),
       ShouldUpdateLOSStamp);
}

#pragma endregion

//======================= LOS Target Detection =======================================================================//
#pragma region LOS Target Detection Management

void ULineOfSightComponent::UpdateTargetDetection()
{
    if (!bDetectionEnabled || !VisionSphere || !VisibilityTracer)
    {
        UE_LOG(LOSVision, VeryVerbose,
            TEXT("[%s] ULineOfSightComponent::UpdateTargetDetection >> skipped (Disabled or missing components)"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    const FVector ObserverLocation = GetOwner()->GetActorLocation();

    if (bDrawDetectionDebug)//draw a debug sphere for collision range
    {
        DrawDebugSphere(
            GetWorld(),
            ObserverLocation,
            VisionRange,
            24,// segments
            FColor::Blue,
            false, // persistent
            -1.f,// lifetime
            0,// depth priority
            2.f // thickness
        );
    }
    
    for (auto& Pair : TargetVisibilityMap)
    {
        AActor* TargetActor = Pair.Key;
        bool bLastVisible = Pair.Value;

        if (!TargetActor || TargetActor == GetOwner())
            continue;

        UPrimitiveComponent* TargetShape = ResolveVisibilityShape(TargetActor);
        if (!TargetShape)
        {
            UE_LOG(LOSVision, Warning,
                TEXT("[%s] ULineOfSightComponent::UpdateTargetDetection >> Cannot resolve visibility shape: %s"),
                *TopDownVisionDebug::GetClientDebugName(GetOwner()),
                *TargetActor->GetName());
            continue;
        }

        bool bCurrentlyVisible = VisibilityTracer->IsTargetVisible(
            GetWorld(),
            ObserverLocation,
            TargetShape,
            VisionRange,
            ObstacleTraceChannel,
            { GetOwner() },
            bDrawDetectionDebug,
            DesiredAngleDegree);

        // Only call handler if visibility changed
        if (bCurrentlyVisible != bLastVisible)
        {
            UE_LOG(LOSVision, Log,
               TEXT("[%s] VisibilityChanged: %s | WasVisible=%d | NowVisible=%d"),
               *TopDownVisionDebug::GetClientDebugName(GetOwner()),
               *TargetActor->GetName(),
               bLastVisible,
               bCurrentlyVisible);
            
            HandleTargetVisibilityChanged(TargetActor, bCurrentlyVisible);
            Pair.Value = bCurrentlyVisible; // update map
        }
        else
        {
            UE_LOG(LOSVision, VeryVerbose,
                TEXT("[%s] NoChange in visibility: %s | Visible=%d"),
                *TopDownVisionDebug::GetClientDebugName(GetOwner()),
                *TargetActor->GetName(),
                bCurrentlyVisible);
        }
    }
}

void ULineOfSightComponent::OnVisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == GetOwner() || !OtherComp)
        return;

    
    // Only react to the tagged body component
    if (!OtherComp->ComponentHasTag(VisionTargetTag))
    {
        UE_LOG(LOSVision, Verbose,
            TEXT("[%s] ULineOfSightComponent::OnVisionSphereBeginOverlap >> OverlapBegin ignored (missing tag)"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }
    
    if (!TargetVisibilityMap.Contains(OtherActor))
    {
        TargetVisibilityMap.Add(OtherActor, false);

        UE_LOG(LOSVision, Log,
            TEXT("[%s] ULineOfSightComponent::OnVisionSphereBeginOverlap >> LOS overlap begin (BodyComp validated): %s"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()),
            *OtherActor->GetName());
    }
}

void ULineOfSightComponent::OnVisionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!OtherActor || !OtherComp)
        return;
    
    if (!OtherComp->ComponentHasTag(VisionTargetTag))
        return;
    
    if (TargetVisibilityMap.Contains(OtherActor))
    {
        TargetVisibilityMap.Remove(OtherActor);

        HandleTargetVisibilityChanged(OtherActor, false);

        UE_LOG(LOSVision, Log,
            TEXT("[%s] ULineOfSightComponent::OnVisionSphereEndOverlap >> LOS overlap end (BodyComp validated): %s"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()),
            *OtherActor->GetName());
    }
}

UPrimitiveComponent* ULineOfSightComponent::ResolveVisibilityShape(AActor* TargetActor) const
{
    if (!TargetActor)
        return nullptr;

    TArray<UPrimitiveComponent*> Prims;
    TargetActor->GetComponents<UPrimitiveComponent>(Prims);

    for (UPrimitiveComponent* Comp : Prims)
    {
        if (Comp && Comp->ComponentHasTag(VisionTargetTag))
            return Comp;
    }

    return nullptr;
}


void ULineOfSightComponent::HandleTargetVisibilityChanged(AActor* DetectedTarget, bool bIsVisible)
{
    if (!DetectedTarget)
        return;

    // Get GameState // using lazy load method
    UVisionGameStateComp* VisionGSComp = GetVisionGameStateComp();
    if (!VisionGSComp)
        return;

    //Get LOS Vision Subsystem
    ULOSVisionSubsystem* VisionSubsystem=GetLOSVisionSubsystem();
    if (!VisionSubsystem)
        return;
    
    // Register / Unregister target visibility
    if (bIsVisible)
    {
        VisionGSComp->RegisterVisionProvider(this);//register to the GameStateComp
        VisionSubsystem->RegisterProvider(this,VisionChannel);//Register to the Subsystem
        
        UE_LOG(LOSVision, Log,
            TEXT("[%s] ULineOfSightComponent::HandleTargetVisibilityChanged >> Registered visible target: %s"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()),
            *DetectedTarget->GetName());
        
    }
    else
    {
        VisionGSComp->UnregisterVisionProvider(this);//unregister from the GameStateComp
        VisionSubsystem->UnregisterProvider(this,VisionChannel);//Unregister from the Subsystem
        
        UE_LOG(LOSVision, Log,
            TEXT("[%s] ULineOfSightComponent::HandleTargetVisibilityChanged >> Unregistered target: %s"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()),
            *DetectedTarget->GetName());
    }
}

UVisionGameStateComp* ULineOfSightComponent::GetVisionGameStateComp()
{
    // Already cached, just return
    if (CachedVisionGameStateComp)
        return CachedVisionGameStateComp;

    // Get GameState
    if (UWorld* World = GetWorld())
    {
        if (AGameStateBase* GS = World->GetGameState())
        {
            CachedVisionGameStateComp = GS->FindComponentByClass<UVisionGameStateComp>();
            if (!CachedVisionGameStateComp)
            {
                UE_LOG(LOSVision, Warning,
                    TEXT("[%s] ULineOfSightComponent::GetVisionGameStateComp >> UVisionGameStateComp not found on GameState"),
                    *TopDownVisionDebug::GetClientDebugName(GetOwner()));
            }
        }
    }

    return CachedVisionGameStateComp;
}

ULOSVisionSubsystem* ULineOfSightComponent::GetLOSVisionSubsystem()
{
    if (!GetWorld())
    {
        return nullptr;
    }

    return GetWorld()->GetSubsystem<ULOSVisionSubsystem>();
}

#pragma endregion






