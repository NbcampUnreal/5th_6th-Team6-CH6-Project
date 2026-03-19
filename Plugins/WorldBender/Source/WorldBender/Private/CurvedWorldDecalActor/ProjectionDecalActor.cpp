// Fill out your copyright notice in the Description page of Project Settings.

#include "CurvedWorldDecalActor/ProjectionDecalActor.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AProjectionDecalActor::AProjectionDecalActor()
{
    PrimaryActorTick.bCanEverTick = false;

    BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
    RootComponent = BoxMesh;

    // Use engine cube
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        BoxMesh->SetStaticMesh(CubeMesh.Object);
    }

    BoxMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BoxMesh->SetGenerateOverlapEvents(false);
    BoxMesh->SetCastShadow(false);
}

void AProjectionDecalActor::BeginPlay()
{
    Super::BeginPlay();
    InitMID();
    UpdateProjectionParams();
}

void AProjectionDecalActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    InitMID();

    // Match mesh scale to projection extent
    FVector Scale = ProjectionExtent / 50.f; // Cube is 100 units
    BoxMesh->SetWorldScale3D(Scale);

    UpdateProjectionParams();
}

#if WITH_EDITOR
void AProjectionDecalActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    UpdateProjectionParams();
}
#endif

void AProjectionDecalActor::InitMID()
{
    if (!BaseMaterial || !BoxMesh)
        return;

    if (!MID)
    {
        MID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        BoxMesh->SetMaterial(0, MID);
    }
}

void AProjectionDecalActor::UpdateProjectionParams()
{
    if (!MID)
        return;

    const FVector Center  = GetActorLocation();
    const FVector Right   = GetActorRightVector().GetSafeNormal();
    const FVector Forward = GetActorForwardVector().GetSafeNormal();
    const FVector Up      = GetActorUpVector().GetSafeNormal();

    MID->SetVectorParameterValue(Param_ProjectionCenter, FLinearColor(Center));
    MID->SetVectorParameterValue(Param_ProjectionRight, FLinearColor(Right));
    MID->SetVectorParameterValue(Param_ProjectionForward, FLinearColor(Forward));
    MID->SetVectorParameterValue(Param_ProjectionUp, FLinearColor(Up));

    MID->SetVectorParameterValue(
        TEXT("ProjScale"),
        FLinearColor(ProjectionExtent.X, ProjectionExtent.Y, ProjectionExtent.Z, 0.f)
    );
}