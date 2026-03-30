#include "Debug/RTDebugComponent.h"
#include "Managers/RTPoolManager.h"
#include "Data/RTPoolTypes.h"
#include "Debug/LogCategory.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

const FName URTDebugComponent::PN_DebugTex(TEXT("DebugTex"));
const FName URTDebugComponent::PN_DebugChannel(TEXT("DebugChannel"));

URTDebugComponent::URTDebugComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup   = TG_PostUpdateWork;
}

void URTDebugComponent::BeginPlay()
{
    Super::BeginPlay();

    UWorld* World = GetOwner() ? GetOwner()->GetWorld() : nullptr;
    if (!World)
    {
        UE_LOG(RTFoliageInvoker, Warning,
            TEXT("URTDebugComponent::BeginPlay >> World is null — debug disabled"));
        return;
    }

    PoolManager = World->GetSubsystem<URTPoolManager>();
    if (!PoolManager)
    {
        UE_LOG(RTFoliageInvoker, Warning,
            TEXT("URTDebugComponent::BeginPlay >> URTPoolManager not found — debug disabled"));
        SetComponentTickEnabled(false);
        return;
    }

    if (bShowDebugPlanes)
        ResizePlanePool(PoolManager->PoolSize);

    const FVector OwnerLoc = GetOwner()->GetActorLocation();
    UE_LOG(RTFoliageInvoker, Log,
        TEXT("URTDebugComponent::BeginPlay >> Initialised — PoolSize=%d bBoxes=%s bPlanes=%s  OwnerLoc(%.0f,%.0f,%.0f)"),
        PoolManager->PoolSize,
        bShowDebugBoxes  ? TEXT("true") : TEXT("false"),
        bShowDebugPlanes ? TEXT("true") : TEXT("false"),
        OwnerLoc.X, OwnerLoc.Y, OwnerLoc.Z);
}

void URTDebugComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    for (UStaticMeshComponent* Plane : DebugPlanes)
    {
        if (Plane) { Plane->DestroyComponent(); }
    }
    DebugPlanes.Empty();
    PlaneMIDs.Empty();

    const FVector OwnerLoc = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
    UE_LOG(RTFoliageInvoker, Log,
        TEXT("URTDebugComponent::EndPlay >> Debug planes destroyed  OwnerLoc(%.0f,%.0f,%.0f)"),
        OwnerLoc.X, OwnerLoc.Y, OwnerLoc.Z);

    Super::EndPlay(EndPlayReason);
}

void URTDebugComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!PoolManager) { return; }
    if (bShowDebugBoxes)  { DrawBoxes();    }
    if (bShowDebugPlanes) { UpdatePlanes(); }
}

void URTDebugComponent::RefreshDebugPlanes()
{
    if (!PoolManager)
    {
        UE_LOG(RTFoliageInvoker, Warning,
            TEXT("URTDebugComponent::RefreshDebugPlanes >> PoolManager is null"));
        return;
    }
    ResizePlanePool(PoolManager->PoolSize);
    UpdatePlanes();

    const FVector OwnerLoc = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
    UE_LOG(RTFoliageInvoker, Log,
        TEXT("URTDebugComponent::RefreshDebugPlanes >> Refreshed PoolSize=%d  OwnerLoc(%.0f,%.0f,%.0f)"),
        PoolManager->PoolSize, OwnerLoc.X, OwnerLoc.Y, OwnerLoc.Z);
}

void URTDebugComponent::DrawBoxes()
{
    UWorld* World = GetOwner() ? GetOwner()->GetWorld() : nullptr;
    if (!World) { return; }

    const TArray<FRTPoolEntry>& Pool = PoolManager->GetPool();
    const float HalfSize = PoolManager->CellSize * 0.5f;

    for (int32 i = 0; i < Pool.Num(); ++i)
    {
        const FRTPoolEntry& Slot = Pool[i];
        if (!Slot.IsOccupied()) { continue; }

        const FVector Centre(
            Slot.CellOriginWS.X + HalfSize,
            Slot.CellOriginWS.Y + HalfSize,
            PlaneHeight);

        DrawDebugBox(World, Centre, FVector(HalfSize, HalfSize, 2.f),
            ActiveBoxColor, false, -1.f, 0, 3.f);
        DrawDebugString(World, Centre + FVector(0, 0, 40.f),
            FString::Printf(TEXT("Slot %d"), i),
            nullptr, ActiveBoxColor, 0.f, true);
    }
}

void URTDebugComponent::UpdatePlanes()
{
    const TArray<FRTPoolEntry>& Pool = PoolManager->GetPool();
    if (DebugPlanes.Num() < Pool.Num()) { ResizePlanePool(Pool.Num()); }
    for (int32 i = 0; i < Pool.Num(); ++i)
    {
        Pool[i].IsOccupied() ? UpdatePlane(i, i) : HidePlane(i);
    }
}

void URTDebugComponent::UpdatePlane(int32 PlaneIndex, int32 SlotIndex)
{
    if (!DebugPlanes.IsValidIndex(PlaneIndex)) { return; }

    UStaticMeshComponent*     Plane = DebugPlanes[PlaneIndex];
    UMaterialInstanceDynamic* MID   = PlaneMIDs.IsValidIndex(PlaneIndex)
                                    ? PlaneMIDs[PlaneIndex] : nullptr;
    if (!Plane) { return; }

    const TArray<FRTPoolEntry>& Pool = PoolManager->GetPool();
    if (!Pool.IsValidIndex(SlotIndex)) { return; }

    const FRTPoolEntry& Slot     = Pool[SlotIndex];
    const float         CellSize = PoolManager->CellSize;
    const float         HalfSize = CellSize * 0.5f;

    Plane->SetWorldLocation(FVector(
        Slot.CellOriginWS.X + HalfSize,
        Slot.CellOriginWS.Y + HalfSize,
        PlaneHeight));
    Plane->SetWorldRotation(FRotator::ZeroRotator);
    Plane->SetWorldScale3D(FVector(CellSize / 100.f, CellSize / 100.f, 1.f));
    Plane->SetVisibility(true);

    if (MID)
    {
        MID->SetTextureParameterValue(PN_DebugTex, Slot.InteractionRT);
        MID->SetScalarParameterValue(PN_DebugChannel, static_cast<float>(DebugChannel));
    }
}

void URTDebugComponent::ResizePlanePool(int32 Count)
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        UE_LOG(RTFoliageInvoker, Warning,
            TEXT("URTDebugComponent::ResizePlanePool >> Owner is null"));
        return;
    }

    const int32 PrevCount = DebugPlanes.Num();

    while (DebugPlanes.Num() < Count)
    {
        UStaticMeshComponent* Plane = NewObject<UStaticMeshComponent>(Owner);
        Plane->RegisterComponent();

        if (PlaneMesh)
        {
            Plane->SetStaticMesh(PlaneMesh);
        }
        else
        {
            UStaticMesh* DefaultPlane = Cast<UStaticMesh>(StaticLoadObject(
                UStaticMesh::StaticClass(), nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
            if (DefaultPlane) { Plane->SetStaticMesh(DefaultPlane); }
        }

        Plane->SetVisibility(false);
        Plane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Plane->SetCastShadow(false);

        UMaterialInstanceDynamic* MID = nullptr;
        if (DebugPlaneMaterial)
        {
            MID = UMaterialInstanceDynamic::Create(DebugPlaneMaterial, Owner);
            Plane->SetMaterial(0, MID);
        }

        DebugPlanes.Add(Plane);
        PlaneMIDs.Add(MID);
    }

    if (DebugPlanes.Num() > PrevCount)
    {
        UE_LOG(RTFoliageInvoker, Verbose,
            TEXT("URTDebugComponent::ResizePlanePool >> Grew %d → %d"),
            PrevCount, DebugPlanes.Num());
    }

    for (int32 i = Count; i < DebugPlanes.Num(); ++i)
    {
        if (DebugPlanes[i]) { DebugPlanes[i]->SetVisibility(false); }
    }
}

void URTDebugComponent::HidePlane(int32 PlaneIndex)
{
    if (DebugPlanes.IsValidIndex(PlaneIndex) && DebugPlanes[PlaneIndex])
        DebugPlanes[PlaneIndex]->SetVisibility(false);
}