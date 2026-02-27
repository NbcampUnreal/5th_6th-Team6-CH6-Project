// Fill out your copyright notice in the Description page of Project Settings.


#include "OcclusionProbeComp.h"

#include "FCurvedWorldUtil.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"


// Sets default values for this component's properties
UOcclusionProbeComp::UOcclusionProbeComp()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UOcclusionProbeComp::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        CameraManager = PC->PlayerCameraManager;
    }

    CurvedWorldSubsystem = GetWorld()->GetSubsystem<UCurvedWorldSubsystem>();

    InitializeProbes();
}


void UOcclusionProbeComp::InitializeProbes()
{
    ProbeSpheres.Reserve(NumProbes);

    for (int32 i = 0; i < NumProbes; ++i)
    {
        FString Name = FString::Printf(TEXT("OcclusionProbe_%d"), i);

        USphereComponent* Sphere = NewObject<USphereComponent>(GetOwner(), *Name);
        Sphere->SetupAttachment(this);
        Sphere->RegisterComponent();

        Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);

        // Overlap both static and dynamic world geometry so walls/pillars are detected
        Sphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
        Sphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);

        // Hide in game — these are logic-only probes
        Sphere->SetHiddenInGame(true);

        ProbeSpheres.Add(Sphere);
    }
}

void UOcclusionProbeComp::TickComponent(float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    UpdateProbes();
}


void UOcclusionProbeComp::UpdateProbes()
{
    if (!CameraManager || !CurvedWorldSubsystem)
        return;

    FVector CameraLoc = CameraManager->GetCameraLocation();
    FVector TargetLoc = GetComponentLocation();

    TArray<FVector> PathPoints =
        FCurvedWorldUtil::GenerateCurvedPathPoints(
            CameraLoc,
            TargetLoc,
            NumProbes,
            CurvedWorldSubsystem,
            ECurveMathType::ZHeightOnly);

    float SphereRadius = CalculateScreenProjectedRadius();

    // Guard: only iterate up to the smallest of the two arrays
    int32 Count = FMath::Min(ProbeSpheres.Num(), PathPoints.Num());

    for (int32 i = 0; i < Count; ++i)
    {
        ProbeSpheres[i]->SetWorldLocation(PathPoints[i]);
        ProbeSpheres[i]->SetSphereRadius(SphereRadius);
    }
}


float UOcclusionProbeComp::CalculateScreenProjectedRadius() const
{
    if (!CameraManager)
        return MinSphereRadius;

    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC)
        return MinSphereRadius;

    // Get actual viewport dimensions for correct pixel->NDC conversion
    int32 ViewportX, ViewportY;
    PC->GetViewportSize(ViewportX, ViewportY);
    if (ViewportX == 0 || ViewportY == 0)
        return MinSphereRadius;

    FVector ActorLocation = GetOwner()->GetActorLocation();

    // Use bounding box to approximate the actor's "radius" in world space
    FBox TargetBounds = GetOwner()->GetComponentsBoundingBox();
    float ApproxRadius = TargetBounds.GetExtent().Size();

    // Project center and an offset point to screen to measure pixel footprint
    FVector WorldEdge = ActorLocation + FVector(ApproxRadius, 0.f, 0.f);

    FVector2D ScreenCenter, ScreenEdge;
    if (!PC->ProjectWorldLocationToScreen(ActorLocation, ScreenCenter) ||
        !PC->ProjectWorldLocationToScreen(WorldEdge, ScreenEdge))
        return MinSphereRadius;

    // How many pixels does ApproxRadius occupy on screen?
    float PixelRadius = FVector2D::Distance(ScreenCenter, ScreenEdge);

    // Normalize to NDC using viewport height so aspect ratio doesn't skew things
    float NDCRadius = PixelRadius / static_cast<float>(ViewportY);

    // Back-project NDC fraction into world units at the actor's depth
    float DistanceToCamera = FVector::Distance(
        ActorLocation,
        CameraManager->GetCameraLocation());

    float HalfFOVRad = FMath::DegreesToRadians(CameraManager->GetFOVAngle() * 0.5f);

    // Full frustum height at this distance
    float FrustumHeight = 2.f * DistanceToCamera * FMath::Tan(HalfFOVRad);

    // World radius that matches the screen footprint of the actor
    float WorldRadius = NDCRadius * FrustumHeight;

    return FMath::Clamp(WorldRadius, MinSphereRadius, MaxSphereRadius);
}
