#include "Test/TestGridGenerator.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

// Log
DEFINE_LOG_CATEGORY(GridTesterLog);

ATestGridGenerator::ATestGridGenerator()
{
    PrimaryActorTick.bCanEverTick = false;

    // Create Instanced Static Mesh Component
    ISMComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("HexISM"));
    ISMComponent->SetupAttachment(RootComponent);
    ISMComponent->SetMobility(EComponentMobility::Static);

    // Create generator instance
    Generator = NewObject<UHexGridGenerator>();
    UE_LOG(GridTesterLog, Log,
        TEXT("ATestGridGenerator::Constructor >> Generator created"));
}

FHexGridLayout ATestGridGenerator::CreateLayoutAtCenter(const FVector& Center)
{
    FHexGridLayout Layout;
    Layout.HexSize = HexSize;
    Layout.Orientation = Orientation;
    //Layout.Origin = FVector2D(Center.X, Center.Y);

    FVector RootLocation=GetActorLocation();
    Layout.Origin=FVector2D(RootLocation.X, RootLocation.Y);
    
    return Layout;
}

// --- BP-callable generation functions --- //

void ATestGridGenerator::GenerateHexGrid_NoMask()
{
    if (!Generator)
        Generator = NewObject<UHexGridGenerator>();// make new one if the constructor failed to create

    FHexGridWrapper GridWrapper;//empty
    FHexGridLayout Layout = CreateLayoutAtCenter(GetActorLocation());

    bool bSuccess = Generator->GenerateHexagonGrid(Layout, FHexCoord::FromAxial(0,0), GridRadius, GridWrapper);

    if (bSuccess)
    {
        GenerateHexGrid_Internal(GridWrapper.Grid, GetActorLocation());
        UE_LOG(GridTesterLog, Log, TEXT("Generated hex grid with %d hexes"), GridWrapper.Grid.Cells.Num());
    }
}

void ATestGridGenerator::GenerateHexGrid_Circular(const FVector& CenterLocation, float Radius)
{
    
}

void ATestGridGenerator::GenerateHexGrid_Polygon(const TArray<FVector2D>& PolygonPoints)
{
}

void ATestGridGenerator::GenerateHexGrid_Texture(UTexture2D* MaskTexture, float Threshold)
{
    /*if (!MaskTexture)
    {
        UE_LOG(GridTesterLog, Warning,
            TEXT("ATestGridGenerator::GenerateHexGrid_Texture >> MaskTexture is null"));
        return;
    }

    UE_LOG(GridTesterLog, Log,
        TEXT("ATestGridGenerator::GenerateHexGrid_Texture >> Generating hex grid with texture mask, threshold=%.2f"),
        Threshold);

    FHexGrid Grid = Generator->GenerateHexagonGrid(GridRadius);

    FHexGridLayout Layout = CreateLayoutAtCenter(GetActorLocation());
    Generator->ApplyTextureMask(Grid, Layout, MaskTexture, Threshold);

    UE_LOG(GridTesterLog, Log,
        TEXT("ATestGridGenerator::GenerateHexGrid_Texture >> Hexes after mask: %d"),
        Grid.Cells.Num());

    GenerateHexGrid_Internal(Grid, GetActorLocation());*/
}

void ATestGridGenerator::GenerateHexGrid_Square(const FVector& CenterLocation, float X_HalfWidth, float Y_HalfHeight)
{
    /*UE_LOG(GridTesterLog, Log,
        TEXT("ATestGridGenerator::GenerateHexGrid_Square >> Generating square grid, X_HalfWidth=%.2f, Y_HalfHeight=%.2f"),
        X_HalfWidth, Y_HalfHeight);

    FHexGrid Grid = Generator->GenerateSquareGrid(CenterLocation, X_HalfWidth, Y_HalfHeight, HexSize);

    UE_LOG(GridTesterLog, Log,
        TEXT("ATestGridGenerator::GenerateHexGrid_Square >> Base square grid hex count: %d"),
        Grid.Cells.Num());

    GenerateHexGrid_Internal(Grid, CenterLocation);

    UE_LOG(GridTesterLog, Log,
        TEXT("ATestGridGenerator::GenerateHexGrid_Square >> Total hexes spawned: %d"),
        Grid.Cells.Num());*/
}

// --- Clear previous instances --- //
void ATestGridGenerator::ClearHexGrid()
{
    if (ISMComponent)
    {
        ISMComponent->ClearInstances();
        UE_LOG(GridTesterLog, Log,
            TEXT("ATestGridGenerator::ClearHexGrid >> Cleared all hex instances"));
    }
    else
    {
        UE_LOG(GridTesterLog, Warning,
            TEXT("ATestGridGenerator::ClearHexGrid >> ISMComponent not found"));
    }
}

// --- Internal shared spawning --- //
void ATestGridGenerator::GenerateHexGrid_Internal(FHexGrid& Grid, const FVector& GridCenter)
{
    if (!HexMesh)
    {
        UE_LOG(GridTesterLog, Warning,
            TEXT("ATestGridGenerator::GenerateHexGrid_Internal >> HexMesh not assigned! Aborting spawn."));
        return;
    }

    ClearHexGrid();
    ISMComponent->SetStaticMesh(HexMesh);

    FHexGridLayout Layout = CreateLayoutAtCenter(GridCenter);

    int32 SpawnedCount = 0;

    for (const FHexCoord& Hex : Grid.Cells)
    {
        FVector2D World2D = Layout.HexToWorld(Hex);
        FVector WorldPos = FVector(World2D.X, World2D.Y, GridCenter.Z);

        FTransform InstanceTransform;
        InstanceTransform.SetLocation(WorldPos);

        ISMComponent->AddInstance(InstanceTransform);
        SpawnedCount++;

        UE_LOG(GridTesterLog, Log,
            TEXT("ATestGridGenerator::GenerateHexGrid_Internal >> Hex X=%d Y=%d Z=%d -> World X=%.2f Y=%.2f"),
            Hex.Hex_X, Hex.Hex_Y, Hex.Hex_Z, WorldPos.X, WorldPos.Y);
    }

    UE_LOG(GridTesterLog, Log,
        TEXT("ATestGridGenerator::GenerateHexGrid_Internal >> Spawned %d hex instances"),
        SpawnedCount);
}
