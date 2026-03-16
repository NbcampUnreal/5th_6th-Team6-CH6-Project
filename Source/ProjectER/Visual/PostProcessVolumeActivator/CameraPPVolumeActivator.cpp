#include "CameraPPVolumeActivator.h"

#include "Engine/PostProcessVolume.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/TopDownCameraComp.h"
#include "GameFramework/Actor.h"

UCameraPPVolumeActivator::UCameraPPVolumeActivator()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UCameraPPVolumeActivator::Initialize(UTopDownCameraComp* InCameraComp)
{
	CameraComp = InCameraComp;

	if (!CameraComp)
	{
		UE_LOG(LogTemp, Error,
			TEXT("CameraPPVolumeActivator Initialize failed: invalid camera component"));
		return;
	}

	CacheVolumes();

	SetComponentTickEnabled(true);
}

void UCameraPPVolumeActivator::BeginPlay()
{
	Super::BeginPlay();

	if (!CameraComp)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("CameraPPVolumeActivator: CameraComp not initialized!"));
		return;
	}

	CacheVolumes();
}

void UCameraPPVolumeActivator::CacheVolumes()
{
	ManagedVolumes.Empty();

	TArray<AActor*> FoundVolumes;

	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		APostProcessVolume::StaticClass(),
		FoundVolumes
	);

	for (AActor* Actor : FoundVolumes)
	{
		APostProcessVolume* PP = Cast<APostProcessVolume>(Actor);

		if (!PP)
			continue;

		if (PP->bUnbound) // ignore infinite volumes
			continue;

		FPPVolumeRuntime Entry;
		Entry.Volume = PP;

		ManagedVolumes.Add(Entry);
	}

	UE_LOG(LogTemp, Log,
		TEXT("CameraPPVolumeActivator cached %d PostProcessVolumes"),
		ManagedVolumes.Num());
}

void UCameraPPVolumeActivator::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CameraComp)
		return;

	DetectionTimer += DeltaTime;

	// Run overlap detection at lower frequency
	if (DetectionTimer >= UpdateInterval)
	{
		DetectionTimer = 0.f;
		UpdateVolumes();
	}

	// Smooth blend every frame
	for (FPPVolumeRuntime& Entry : ManagedVolumes)
	{
		APostProcessVolume* Volume = Entry.Volume;

		if (!Volume)
			continue;

		float NewWeight = FMath::FInterpTo(
			Volume->BlendWeight,
			Entry.TargetWeight,
			DeltaTime,
			BlendSpeed
		);

		Volume->BlendWeight = NewWeight;

		// Restore bounds when fully faded out
		if (Entry.TargetWeight == 0.f && NewWeight <= 0.01f)
		{
			Volume->BlendWeight = 0.f;
			Volume->bUnbound = false;
		}
	}
}

void UCameraPPVolumeActivator::UpdateVolumes()
{
	if (!CameraComp)
		return;

	const FVector PivotLocation = CameraComp->GetComponentLocation();

	for (FPPVolumeRuntime& Entry : ManagedVolumes)
	{
		APostProcessVolume* Volume = Entry.Volume;

		if (!Volume)
			continue;

		const bool bInside =
			Volume->EncompassesPoint(PivotLocation, 50.f, nullptr);

		Entry.TargetWeight = bInside ? 1.f : 0.f;

		if (bInside && Volume->BlendWeight <= 0.f)
		{
			Volume->bUnbound = true;
		}
	}
}