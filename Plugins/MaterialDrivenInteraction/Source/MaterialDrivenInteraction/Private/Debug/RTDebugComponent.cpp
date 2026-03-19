// Fill out your copyright notice in the Description page of Project Settings.


#include "Debug/RTDebugComponent.h"
#include "PoolManager/RTPoolManager.h"
#include "Data/RTPoolTypes.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Debug/LogCategory.h"

const FName URTDebugComponent::PN_DebugTex(TEXT("DebugTex"));

// ── Constructor ───────────────────────────────────────────────────────────────

URTDebugComponent::URTDebugComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup   = TG_PostUpdateWork;
}

// ── UActorComponent ───────────────────────────────────────────────────────────

void URTDebugComponent::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World) { return; }

	PoolManager = World->GetSubsystem<URTPoolManager>();
	if (!PoolManager)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RTDebugComponent] URTPoolManager not found — debug disabled."));
		SetComponentTickEnabled(false);
		return;
	}

	// Pre-create one plane per pool slot
	if (bShowDebugPlanes)
	{
		ResizePlanePool(PoolManager->PoolSize);
	}
}

void URTDebugComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Destroy all debug plane components
	for (UStaticMeshComponent* Plane : DebugPlanes)
	{
		if (Plane) { Plane->DestroyComponent(); }
	}
	DebugPlanes.Empty();
	PlaneMIDs.Empty();

	Super::EndPlay(EndPlayReason);
}

void URTDebugComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PoolManager) { return; }

	if (bShowDebugBoxes)  { DrawBoxes();   }
	if (bShowDebugPlanes) { UpdatePlanes(); }
}

// ── Public ────────────────────────────────────────────────────────────────────

void URTDebugComponent::RefreshDebugPlanes()
{
	if (!PoolManager) { return; }
	ResizePlanePool(PoolManager->PoolSize);
	UpdatePlanes();
}

// ── Internal ──────────────────────────────────────────────────────────────────

void URTDebugComponent::DrawBoxes()
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	const TArray<FRTPoolEntry>& Pool = PoolManager->GetPool();
	const float HalfSize = PoolManager->CellSize * 0.5f;

	for (int32 i = 0; i < Pool.Num(); ++i)
	{
		const FRTPoolEntry& Slot = Pool[i];
		if (!Slot.IsOccupied()) { continue; }

		// Centre of the cell in world space
		const FVector Centre(
			Slot.CellOriginWS.X + HalfSize,
			Slot.CellOriginWS.Y + HalfSize,
			PlaneHeight
		);

		const FVector Extent(HalfSize, HalfSize, 2.f);

		DrawDebugBox(World, Centre, Extent, ActiveBoxColor,
			false,   // persistent
			-1.f,    // lifetime — negative = this frame only
			0,       // depth priority
			3.f      // thickness
		);

		// Draw slot index label at the centre
		DrawDebugString(World, Centre + FVector(0, 0, 40.f),
			FString::Printf(TEXT("Slot %d"), i),
			nullptr, ActiveBoxColor, 0.f, true);
	}
}

void URTDebugComponent::UpdatePlanes()
{
	const TArray<FRTPoolEntry>& Pool = PoolManager->GetPool();

	// Grow the plane pool if needed
	if (DebugPlanes.Num() < Pool.Num())
	{
		ResizePlanePool(Pool.Num());
	}

	for (int32 i = 0; i < Pool.Num(); ++i)
	{
		if (Pool[i].IsOccupied())
		{
			UpdatePlane(i, i);
		}
		else
		{
			HidePlane(i);
		}
	}
}

void URTDebugComponent::ResizePlanePool(int32 Count)
{
	AActor* Owner = GetOwner();
	if (!Owner) { return; }

	// Grow
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
			// Fall back to engine default plane
			UStaticMesh* DefaultPlane = Cast<UStaticMesh>(StaticLoadObject(
				UStaticMesh::StaticClass(), nullptr,
				TEXT("/Engine/BasicShapes/Plane.Plane")));
			if (DefaultPlane) { Plane->SetStaticMesh(DefaultPlane); }
		}

		Plane->SetVisibility(false);
		Plane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Plane->SetCastShadow(false);

		// Create MID for this plane
		UMaterialInstanceDynamic* MID = nullptr;
		if (DebugPlaneMaterial)
		{
			MID = UMaterialInstanceDynamic::Create(DebugPlaneMaterial, Owner);
			Plane->SetMaterial(0, MID);
		}

		DebugPlanes.Add(Plane);
		PlaneMIDs.Add(MID);
	}

	// Shrink — hide excess
	for (int32 i = Count; i < DebugPlanes.Num(); ++i)
	{
		if (DebugPlanes[i]) { DebugPlanes[i]->SetVisibility(false); }
	}
}

void URTDebugComponent::UpdatePlane(int32 PlaneIndex, int32 SlotIndex)
{
	if (!DebugPlanes.IsValidIndex(PlaneIndex)) { return; }

	UStaticMeshComponent* Plane = DebugPlanes[PlaneIndex];
	UMaterialInstanceDynamic* MID = PlaneMIDs.IsValidIndex(PlaneIndex)
		? PlaneMIDs[PlaneIndex] : nullptr;

	if (!Plane) { return; }

	const TArray<FRTPoolEntry>& Pool = PoolManager->GetPool();
	if (!Pool.IsValidIndex(SlotIndex)) { return; }

	const FRTPoolEntry& Slot = Pool[SlotIndex];
	const float CellSize     = PoolManager->CellSize;
	const float HalfSize     = CellSize * 0.5f;

	// Position plane at cell centre, flat on XY, at PlaneHeight
	const FVector Location(
		Slot.CellOriginWS.X + HalfSize,
		Slot.CellOriginWS.Y + HalfSize,
		PlaneHeight
	);

	// Engine plane mesh is 100x100 units — scale to match CellSize
	const float Scale = CellSize / 100.f;

	Plane->SetWorldLocation(Location);
	Plane->SetWorldRotation(FRotator(0.f, 0.f, 0.f));
	Plane->SetWorldScale3D(FVector(Scale, Scale, 1.f));
	Plane->SetVisibility(true);

	// Push the correct RT to the MID
	if (MID)
	{
		UTextureRenderTarget2D* RT = bShowImpulseRT
			? Slot.ImpulseRT
			: Slot.ContinuousRT;

		MID->SetTextureParameterValue(PN_DebugTex, RT);
	}
}

void URTDebugComponent::HidePlane(int32 PlaneIndex)
{
	if (!DebugPlanes.IsValidIndex(PlaneIndex)) { return; }
	if (DebugPlanes[PlaneIndex])
	{
		DebugPlanes[PlaneIndex]->SetVisibility(false);
	}
}
