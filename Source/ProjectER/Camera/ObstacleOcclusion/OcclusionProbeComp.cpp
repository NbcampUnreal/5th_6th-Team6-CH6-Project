#include "OcclusionProbeComp.h"

#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "DrawDebugHelpers.h"

UOcclusionProbeComp::UOcclusionProbeComp()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UOcclusionProbeComp::BeginPlay()
{
    Super::BeginPlay();

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        CameraManager = PC->PlayerCameraManager;
        bCameraReady  = IsValid(CameraManager);
    }

    InitializeProbes();
}

void UOcclusionProbeComp::SetCameraManager(APlayerCameraManager* InCameraManager)
{
    CameraManager = InCameraManager;
    bCameraReady  = IsValid(CameraManager);
}

void UOcclusionProbeComp::InitializeProbes()
{
    // Pre-allocate max probe count — probes are hidden/shown as needed
    ProbeSpheres.Reserve(MaxProbeCount);

    for (int32 i = 0; i < MaxProbeCount; ++i)
    {
        const FString Name = FString::Printf(TEXT("OcclusionProbe_%d"), i);

        USphereComponent* Sphere = NewObject<USphereComponent>(GetOwner(), *Name);
        Sphere->SetupAttachment(this);
        Sphere->RegisterComponent();

        Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
        Sphere->SetCollisionResponseToChannel(OcclusionProbeChannel, ECR_Overlap);

        // Hidden by default — only shown in debug mode
        Sphere->SetHiddenInGame(!bDebugDraw);

        ProbeSpheres.Add(Sphere);
    }
}

void UOcclusionProbeComp::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bCameraReady) return;

    UpdateProbes();
}

bool UOcclusionProbeComp::RefreshFrustumParams()
{
    if (!IsValid(CameraManager)) return false;

    return FFrustumProjectionMatcherHelper::ExtractFromCameraManager(CameraManager, FrustumParams);
}

void UOcclusionProbeComp::UpdateProbes()
{
    if (!RefreshFrustumParams()) return;

    const FVector CameraLoc  = FrustumParams.CameraLocation;
    const FVector TargetLoc  = GetComponentLocation();
    const FVector LineDir    = (TargetLoc - CameraLoc).GetSafeNormal();
    const float   LineLength = FVector::Dist(CameraLoc, TargetLoc);

    // Hide all probes first — only active ones will be repositioned
    for (USphereComponent* Sphere : ProbeSpheres)
    {
        Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        if (bDebugDraw) Sphere->SetHiddenInGame(true);
    }

    // Walk from target outward toward camera — same logic as GenerateSweepsAlongLine
    float DistToTarget = 0.f;
    int32 ProbeIdx     = 0;

    while (ProbeIdx < MaxProbeCount)
    {
        const float DepthFromCamera = LineLength - DistToTarget;

        if (DepthFromCamera <= 0.f) break;

        float Radius = 0.f;

        if (RocketHeadDistance > 0.f && DistToTarget <= RocketHeadDistance)
        {
            // Rocket head zone — exponent taper near target
            const float NormalizedDist = FMath::Clamp(
                DistToTarget / RocketHeadDistance, 0.f, 1.f);

            Radius = FMath::Pow(NormalizedDist, RocketHeadExponent) * TargetVisibleRadius;
            Radius = FMath::Max(Radius, 1.f);
        }
        else
        {
            // Normal frustum projection matching outside rocket head zone
            Radius = FFrustumProjectionMatcherHelper::CalculateSphereRadiusAtDepth(
                FrustumParams,
                LineLength,
                TargetVisibleRadius,
                DepthFromCamera);

            if (Radius <= KINDA_SMALL_NUMBER) break;
        }

        // Cap radius so sphere never reaches past the target
        const float ClampedRadius = FMath::Min(Radius, DistToTarget > 0.f ? DistToTarget : 1.f);

        // Place probe on the straight camera-to-target line at this depth
        const FVector ProbeLocation = CameraLoc + LineDir * DepthFromCamera;

        USphereComponent* Sphere = ProbeSpheres[ProbeIdx];
        Sphere->SetWorldLocation(ProbeLocation);
        Sphere->SetSphereRadius(ClampedRadius);
        Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

        if (bDebugDraw)
        {
            Sphere->SetHiddenInGame(false);

            DrawDebugSphere(
                GetWorld(),
                ProbeLocation,
                ClampedRadius,
                8,
                DistToTarget <= RocketHeadDistance ? FColor::Orange : FColor::Red,
                false,
                -1.f);
        }

        // Step away from target
        DistToTarget += Radius * SweepGapRatio;
        ++ProbeIdx;
    }
}