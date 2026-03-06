// Fill out your copyright notice in the Description page of Project Settings.


#include "ObstacleOcclusion/MainOcclusionPainter.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Canvas.h"
#include "Engine/TextureRenderTarget2D.h"
#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"
#include "Materials/MaterialInstanceDynamic.h"

UMainOcclusionPainter::UMainOcclusionPainter()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UMainOcclusionPainter::BeginPlay()
{
    Super::BeginPlay();

    if (MaskingMateiral)
    {
        MaskingMateiralMID = UMaterialInstanceDynamic::Create(MaskingMateiral, this);
    }
}

void UMainOcclusionPainter::UpdateOcclusionRT()
{
    if (!OcclusionRT || !MaskingMateiralMID) return;

    // 1. Clear the Render Target to Black (Full Occlusion)
    UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), OcclusionRT, FLinearColor::Black);

    // 2. Start Drawing to Canvas
    UCanvas* Canvas;
    FVector2D Size;
    FDrawToRenderTargetContext Context;
    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(GetWorld(), OcclusionRT, Canvas, Size, Context);

    if (Canvas)
    {
        DrawProviderArea();
    }

    // 3. Finish and Resolve
    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);
}

void UMainOcclusionPainter::DrawProviderArea()
{
    /*ULOSVisionSubsystem* Subsystem = GetWorld()->GetSubsystem<ULOSVisionSubsystem>();
    if (!Subsystem || !OcclusionRT) return;

    // Reuse your team-based provider logic
    TArray<ULineOfSightComponent*> Providers = Subsystem->GetProvidersForTeam(VisionChannel);
    
    UCanvas* Canvas;
    FVector2D Size;
    FDrawToRenderTargetContext Context;
    
    // We begin drawing to the OCCLUSION Render Target
    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(GetWorld(), OcclusionRT, Canvas, Size, Context);

    if (Canvas)
    {
        // 1. Clear to Black (Occluded)
        UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), OcclusionRT, FLinearColor::Black);

        for (ULineOfSightComponent* Provider : Providers)
        {
            if (!Provider || !Provider->IsUpdating()) continue;

            FVector2D PixelPos;
            float TileSize;

            // REUSE your existing math function from the Manager logic
            // Note: You may need to make ConvertWorldToRT a static helper or move it to a Library
            if (ConvertWorldToRT(Provider->GetOwner()->GetActorLocation(), 
                                Provider->GetVisibleRange(), 
                                PixelPos, TileSize))
            {
                // We use a specific Masking MID (e.g., a simple soft radial gradient)
                FCanvasTileItem Tile(
                    PixelPos - FVector2D(TileSize * 0.5f, TileSize * 0.5f),
                    MaskingMateiralMID->GetRenderProxy(),
                    FVector2D(TileSize, TileSize)
                );

                // ADDITIVE is key here: multiple players' masks should merge
                Tile.BlendMode = SE_BLEND_Additive;
                Canvas->DrawItem(Tile);
            }
        }
    }
    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);*/
}
