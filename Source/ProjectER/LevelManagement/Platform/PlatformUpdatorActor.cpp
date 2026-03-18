#include "PlatformUpdatorActor.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"

APlatformUpdatorActor::APlatformUpdatorActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void APlatformUpdatorActor::BeginPlay()
{
	Super::BeginPlay();

	InitialLocation = GetActorLocation();
	InitialRotation = GetActorQuat();
}

void APlatformUpdatorActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	PushTransformToMPC();
}

void APlatformUpdatorActor::SetInitialLocation()
{
	InitialLocation = GetActorLocation();
	InitialRotation = GetActorQuat();
	PushTransformToMPC();
}

void APlatformUpdatorActor::PushTransformToMPC()
{
	if (!TargetMPC) return;

	UMaterialParameterCollectionInstance* MPCI =
		GetWorld()->GetParameterCollectionInstance(TargetMPC);
	if (!MPCI) return;

	FTransform T   = GetActorTransform();
	FQuat      Q   = T.GetRotation();
	FVector    Loc = T.GetLocation();
	FVector    Pivot = bUseSelfAsPivot ? InitialLocation : ManualPivot;

	FQuat DeltaQuat = Q * InitialRotation.Inverse();

	MPCI->SetVectorParameterValue("PlatformPivot",
		FLinearColor(Pivot.X, Pivot.Y, Pivot.Z, 0.f));

	MPCI->SetVectorParameterValue("PlatformOffsetWS",
		FLinearColor(Loc.X - Pivot.X, Loc.Y - Pivot.Y, Loc.Z - Pivot.Z, 0.f));

	MPCI->SetVectorParameterValue("PlatformRotationQuat",
		FLinearColor(DeltaQuat.X, DeltaQuat.Y, DeltaQuat.Z, DeltaQuat.W));

	FVector S = T.GetScale3D();
	MPCI->SetVectorParameterValue("PlatformScale",
		FLinearColor(S.X, S.Y, S.Z, 0.f));
}