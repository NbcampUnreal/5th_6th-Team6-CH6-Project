#include "Test/HexGridComponent.h"

DEFINE_LOG_CATEGORY(HexGridComponentLog);

UHexGridComponent::UHexGridComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    ISMComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("HexISM"));
    ISMComponent->SetupAttachment(this);
    ISMComponent->SetMobility(EComponentMobility::Static);

    Generator = CreateDefaultSubobject<UHexGridGenerator>(TEXT("HexGenerator"));
}

FHexGridLayout UHexGridComponent::MakeLayout(const FVector& WorldOrigin) const
{
    FHexGridLayout Layout;
    Layout.HexSize     = HexSize;
    Layout.Orientation = Orientation;

    if (bSnapToWorldGrid)
    {
        // Global lattice rooted at world zero
        // all grids with same size/orientation share the same hex coordinate space
        Layout.Origin = FVector2D::ZeroVector;
    }
    else
    {
        // Free placement: actor location is the grid origin
        Layout.Origin = FVector2D(WorldOrigin.X, WorldOrigin.Y);
    }

    return Layout;
}

void UHexGridComponent::GenerateGrid(FVector WorldOrigin)
{
    if (!Generator)
    {
        UE_LOG(HexGridComponentLog, Warning, TEXT("GenerateGrid >> Generator is null"));
        return;
    }
    if (!HexMesh)
    {
        UE_LOG(HexGridComponentLog, Warning, TEXT("GenerateGrid >> HexMesh not assigned"));
        return;
    }

    ClearGrid();

    FHexGridLayout Layout = MakeLayout(WorldOrigin);
    FVector ResolvedOrigin = WorldOrigin;
    FHexCoord CenterHex = FHexCoord::FromAxial(0, 0);

    if (bSnapToWorldGrid)
    {
        // Find nearest hex center in global lattice to the actor location
        const FVector2D Snapped2D  = Layout.SnapToGrid(FVector2D(WorldOrigin.X, WorldOrigin.Y));
        ResolvedOrigin             = FVector(Snapped2D.X, Snapped2D.Y, WorldOrigin.Z);

        // Grid is centered on whichever hex that snapped position belongs to
        CenterHex = Layout.WorldToHex(Snapped2D);

        UE_LOG(HexGridComponentLog, Log,
            TEXT("GenerateGrid >> Snapped (%.1f, %.1f) -> (%.1f, %.1f)"),
            WorldOrigin.X, WorldOrigin.Y,
            ResolvedOrigin.X, ResolvedOrigin.Y);
    }
    // else: free placement — ResolvedOrigin and CenterHex stay as-is

    const int32 ActualRadius = FMath::Max(0, GridRadius - 1);

    const bool bSuccess = Generator->GenerateHexagonGrid(
        Layout,
        CenterHex,
        ActualRadius,
        CurrentWrapper);

    if (!bSuccess)
    {
        UE_LOG(HexGridComponentLog, Warning, TEXT("GenerateGrid >> GenerateHexagonGrid failed"));
        return;
    }

    SpawnInstances(ResolvedOrigin, Layout);

    UE_LOG(HexGridComponentLog, Log,
        TEXT("GenerateGrid >> %d hexes | Snap=%s | Origin (%.1f, %.1f, %.1f)"),
        CurrentWrapper.Grid.Cells.Num(),
        bSnapToWorldGrid ? TEXT("true") : TEXT("false"),
        ResolvedOrigin.X, ResolvedOrigin.Y, ResolvedOrigin.Z);
}

void UHexGridComponent::ClearGrid()
{
    if (ISMComponent)
        ISMComponent->ClearInstances();

    CurrentWrapper = FHexGridWrapper();
}

void UHexGridComponent::SpawnInstances(const FVector& ResolvedOrigin, const FHexGridLayout& Layout)
{
    ISMComponent->SetStaticMesh(HexMesh);

    for (const FHexCoord& Hex : CurrentWrapper.Grid.Cells)
    {
        const FVector2D World2D = Layout.HexToWorld(Hex);
        const FVector   WorldPos = FVector(World2D.X, World2D.Y, ResolvedOrigin.Z);

        FTransform T;
        T.SetLocation(WorldPos);
        ISMComponent->AddInstance(T);

        UE_LOG(HexGridComponentLog, Verbose,
            TEXT("SpawnInstances >> Hex (%d,%d,%d) -> (%.1f, %.1f, %.1f)"),
            Hex.Hex_X, Hex.Hex_Y, Hex.Hex_Z,
            WorldPos.X, WorldPos.Y, WorldPos.Z);
    }
}