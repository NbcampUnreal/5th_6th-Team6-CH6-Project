#include "CurvedWorldDecalActor/ProjectionDecalActor.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AProjectionDecalActor::AProjectionDecalActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false; // off by default, enabled via bUpdateOnTick

    BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
    RootComponent = BoxMesh;

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

    // Sync tick state with initial bUpdateOnTick value
    SetActorTickEnabled(bUpdateOnTick);
}

void AProjectionDecalActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UpdateProjectionParams();
}

void AProjectionDecalActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    InitMID();

    FVector Scale = ProjectionExtent / 50.f;
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

void AProjectionDecalActor::SetUpdateOnTick(bool bEnable)
{
    bUpdateOnTick = bEnable;
    SetActorTickEnabled(bEnable);
}

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
    MID->SetVectorParameterValue(Param_ProjectionRight,  FLinearColor(Right));
    MID->SetVectorParameterValue(Param_ProjectionForward,FLinearColor(Forward));
    MID->SetVectorParameterValue(Param_ProjectionUp,     FLinearColor(Up));

    MID->SetVectorParameterValue(
        TEXT("ProjScale"),
        FLinearColor(ProjectionExtent.X, ProjectionExtent.Y, ProjectionExtent.Z, 0.f)
    );
}