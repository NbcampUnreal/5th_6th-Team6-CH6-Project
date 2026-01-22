// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/LineOfSightComponent.h"
#include "Engine/World.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "DrawDebugHelpers.h"

ULineOfSightComponent::ULineOfSightComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // Manual draw
}

void ULineOfSightComponent::BeginPlay()
{
    Super::BeginPlay();

    /*if (CanvasRenderTarget)
    {
        CanvasRenderTarget->OnCanvasRenderTargetUpdate.Clear();
        CanvasRenderTarget->OnCanvasRenderTargetUpdate.AddDynamic(this, &ULineOfSightComponent::UpdateRenderTarget);

        // Generate a simple white texture for drawing triangles
        WhiteTexture = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), 1, 1);
    }*/
}

void ULineOfSightComponent::OnComponentCreated()
{
    /*Super::OnComponentCreated();

    if (CanvasRenderTarget)
    {
        CanvasRenderTarget->OnCanvasRenderTargetUpdate.Clear();
        CanvasRenderTarget->OnCanvasRenderTargetUpdate.AddDynamic(this, &ULineOfSightComponent::UpdateRenderTarget);

        if (!WhiteTexture)
        {
            WhiteTexture = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), 1, 1);
        }
    }*/
}

/** Manual call to draw LOS */
void ULineOfSightComponent::DrawLineOfSight()
{
    /*if (!CanvasRenderTarget || !GetOwner()) return;

    // Update MPC with actor location
    if (VisionMPC)
    {
        UMaterialParameterCollectionInstance* MPCInstance = GetWorld()->GetParameterCollectionInstance(VisionMPC);
        if (MPCInstance)
        {
            FVector ActorLoc = GetOwner()->GetActorLocation();
            MPCInstance->SetVectorParameterValue(FName("LOS_Center"), FLinearColor(ActorLoc));
        }
    }

    // Trigger canvas update
    CanvasRenderTarget->UpdateResource();*/
}

/** Draw triangles to canvas using radial line traces */
void ULineOfSightComponent::UpdateRenderTarget(UCanvas* Canvas, int32 Width, int32 Height)
{
    /*if (!Canvas || !GetOwner() || !WhiteTexture) return;

    FVector Center = GetOwner()->GetActorLocation();
    float AngleStep = 360.f / RayCounts;

    TArray<FVector2D> Points;
    Points.Reserve(RayCounts + 1);

    for (int32 i = 0; i <= RayCounts; ++i)
    {
        float AngleDeg = i * AngleStep;
        FRotator Rot(0.f, AngleDeg, 0.f);
        FVector Dir = Rot.RotateVector(FVector::ForwardVector);

        FVector End = Center + Dir * VisionRange;
        FHitResult Hit;

        FCollisionQueryParams Params;
        Params.AddIgnoredActors(IgnoringActors);
        for (UPrimitiveComponent* Comp : IgnoringComponents)
        {
            Params.AddIgnoredComponent(Comp);
        }

        bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Center, End, TraceChannel, Params);
        FVector HitLocation = bHit ? Hit.Location : End;

        float Scale = Width / (VisionRange * 2.f); 
        FVector2D CanvasPos(
            Width * 0.5f + (HitLocation.X - Center.X) * Scale,
            Height * 0.5f - (HitLocation.Y - Center.Y) * Scale // flip Y
        );

        Points.Add(CanvasPos);
    }

    // Draw triangles
    FVector2D CanvasCenter(Width * 0.5f, Height * 0.5f);
    for (int32 i = 0; i < Points.Num() - 1; ++i)
    {
        FCanvasTriangleItem Tri(CanvasCenter, Points[i], Points[i + 1], WhiteTexture->Resource);
        Tri.SetColor(FLinearColor::White);
        Canvas->DrawItem(Tri);
    }*/
}
