#include "OcclusionProbeComp.h"

#include "FCurvedWorldUtil.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

UOcclusionProbeComp::UOcclusionProbeComp()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UOcclusionProbeComp::BeginPlay()
{
    Super::BeginPlay();

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
        CameraManager = PC->PlayerCameraManager;

    CurvedWorldSubsystem = GetWorld()->GetSubsystem<UCurvedWorldSubsystem>();

    InitializeProbes();
}

void UOcclusionProbeComp::InitializeProbes()
{
    ProbeSpheres.Reserve(NumProbes);

    for (int32 i = 0; i < NumProbes; ++i)
    {
        const FString Name = FString::Printf(TEXT("OcclusionProbe_%d"), i);

        USphereComponent* Sphere = NewObject<USphereComponent>(GetOwner(), *Name);
        Sphere->SetupAttachment(this);
        Sphere->RegisterComponent();

        Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);

        // Respond only to the dedicated occlusion channel —
        // prevents missing dynamic obstacle actors
        Sphere->SetCollisionResponseToChannel(OcclusionProbeChannel, ECR_Overlap);

        ProbeSpheres.Add(Sphere);
    }
}

void UOcclusionProbeComp::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    UpdateProbes();
}

void UOcclusionProbeComp::UpdateProbes()
{
    if (!CameraManager || !CurvedWorldSubsystem) return;

    const FVector CameraLoc = CameraManager->GetCameraLocation();
    const FVector TargetLoc = GetComponentLocation();

    TArray<FVector> PathPoints = FCurvedWorldUtil::GenerateCurvedPathPoints(
        CameraLoc,
        TargetLoc,
        NumProbes,
        CurvedWorldSubsystem,
        ECurveMathType::ZHeightOnly);

    const float SphereRadius = CalculateScreenProjectedRadius();

    for (int32 i = 0; i < ProbeSpheres.Num(); ++i)
    {
        ProbeSpheres[i]->SetWorldLocation(PathPoints[i]);
        ProbeSpheres[i]->SetSphereRadius(SphereRadius);
    }
}

float UOcclusionProbeComp::CalculateScreenProjectedRadius() const
{
    if (!CameraManager) return MinSphereRadius;

    const FVector ActorLocation  = GetOwner()->GetActorLocation();
    const FVector CameraLocation = CameraManager->GetCameraLocation();
    const float   DistToCamera   = FVector::Dist(ActorLocation, CameraLocation);

    if (DistToCamera <= KINDA_SMALL_NUMBER) return MinSphereRadius;

    // Use bounding box extent as the world radius of the target
    const FBox  TargetBounds      = GetOwner()->GetComponentsBoundingBox();
    const float WorldRadius = TargetBounds.GetExtent().GetMax();

    // Project angular size of target back to world units at camera distance
    const float AngularSize     = FMath::Atan2(WorldRadius, DistToCamera);
    const float ProjectedRadius = FMath::Tan(AngularSize) * DistToCamera;

    return FMath::Clamp(ProjectedRadius, MinSphereRadius, MaxSphereRadius);
}