// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/LineOfSightComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TopDownVisionDebug.h"// log

//environment texture source
#include "LineOfSight/WorldObstacle/LocalTextureSampler.h"

// Vision helpers
#include "Components/SphereComponent.h"
#include "LineOfSight/ObjectTracing/ShapeAwareVisibilityTracer.h"

#include "DrawDebugHelpers.h"//debug for visualizing the activation
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"


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

void ULineOfSightComponent::UpdateLocalLOS()
{
    UE_LOG(LOSVision, VeryVerbose,
        TEXT("[%s] ULineOfSightComponent::UpdateLocalLOS >> ENTER | Owner=%s | ShouldUpdate=%d"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        *GetOwner()->GetName(),
        ShouldUpdate);
    
    if (!ShouldRunClientLogic())
    {
        return;// not for server
    }

    
    if (!ShouldUpdate)
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

void ULineOfSightComponent::UpdateTargetDetection()
{
    if (!bDetectionEnabled)
    {
        return;// not doing detection 
    }

    
        
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


void ULineOfSightComponent::ToggleUpdate(bool bIsOn)
{
    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] ToggleUpdate | Owner=%s | New=%d"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        *GetOwner()->GetName(),
        bIsOn);
    
    if (ShouldUpdate==bIsOn)
    {
        UE_LOG(LOSVision, Verbose,
            TEXT("[%s]ULineOfSightComponent::ToggleUpdate >> Already set to %s"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()),
            ShouldUpdate ? TEXT("true") : TEXT("false"));
        return;
    }
    
    ShouldUpdate = bIsOn;

    
    UE_LOG(LOSVision, Verbose,
       TEXT("[%s] ToggleUpdate APPLIED | Owner=%s | ShouldUpdate=%d"),
       *TopDownVisionDebug::GetClientDebugName(GetOwner()),
       *GetOwner()->GetName(),
       ShouldUpdate);
}

void ULineOfSightComponent::OnVisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == GetOwner())
        return;

    if (!OtherComp)
        return;

    //filter by tag
    if (!OtherActor->ActorHasTag(VisionTargetTag))
        return;

    OverlappedTargetActors.Add(OtherActor);

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] LOS overlap begin: %s"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        *OtherActor->GetName());
}

void ULineOfSightComponent::OnVisionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!OtherActor)
        return;

    OverlappedTargetActors.Remove(OtherActor);

    //remove the detected target as well in here

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] LOS overlap end: %s"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        *OtherActor->GetName());
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

UPrimitiveComponent* ULineOfSightComponent::ResolveVisibilityShape(AActor* TargetActor) const
{
    if (!TargetActor)
        return nullptr;

    TArray<UPrimitiveComponent*> Prims;
    TargetActor->GetComponents<UPrimitiveComponent>(Prims);

    //priority of the returning primitive comp
    // 1) Explicit tag wins
    for (UPrimitiveComponent* Comp : Prims)
    {
        if (Comp && Comp->ComponentHasTag(VisionTargetTag))
            return Comp;
    }

    // 2) Prefer common collision shapes
    for (UPrimitiveComponent* Comp : Prims)
    {
        if (Comp && (
            Comp->IsA<UCapsuleComponent>() ||
            Comp->IsA<UBoxComponent>() ||
            Comp->IsA<USphereComponent>()))
        {
            return Comp;
        }
    }

    // 3) Fallback to root primitive
    return Cast<UPrimitiveComponent>(TargetActor->GetRootComponent());
}




