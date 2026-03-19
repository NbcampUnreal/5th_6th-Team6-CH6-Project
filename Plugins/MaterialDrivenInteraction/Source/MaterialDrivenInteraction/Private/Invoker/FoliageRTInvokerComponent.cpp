// Fill out your copyright notice in the Description page of Project Settings.


#include "Invoker/FoliageRTInvokerComponent.h"

#include "PoolManager/RTPoolManager.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

// ── Parameter name constants ──────────────────────────────────────────────────
// Centralised so they stay in sync with the material parameter names.

static const FName PN_VelocityX  (TEXT("VelocityX"));
static const FName PN_VelocityY  (TEXT("VelocityY"));
static const FName PN_Weight     (TEXT("Weight"));
static const FName PN_ImpactTime (TEXT("ImpactTime"));

// ── Constructor ───────────────────────────────────────────────────────────────

UFoliageRTInvokerComponent::UFoliageRTInvokerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Tick after the owning actor has moved so velocity delta is correct.
	PrimaryComponentTick.TickGroup   = TG_PostUpdateWork;
}

// ── UActorComponent ───────────────────────────────────────────────────────────

void UFoliageRTInvokerComponent::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World) { return; }

	// Grab the subsystem — it is always valid in game worlds.
	PoolManager = World->GetSubsystem<URTPoolManager>();
	if (!PoolManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FoliageRTInvoker] URTPoolManager subsystem not found."));
		return;
	}

	// Build MIDs so we can push per-frame scalar parameters cheaply.
	if (BrushMaterial_Continuous)
	{
		MID_Continuous = UMaterialInstanceDynamic::Create(BrushMaterial_Continuous, this);
	}
	if (BrushMaterial_Impulse)
	{
		MID_Impulse = UMaterialInstanceDynamic::Create(BrushMaterial_Impulse, this);
	}

	// Seed previous location so the first tick doesn't produce a garbage velocity spike.
	PrevWorldLocation = GetOwner()->GetActorLocation();

	// Register with the pool for the starting cell.
	const FVector StartLoc = GetOwner()->GetActorLocation();
	ActiveSlotIndex = PoolManager->RegisterInvoker(StartLoc);
	CurrentCell     = PoolManager->WorldToCell(StartLoc);
	bRegistered     = (ActiveSlotIndex != INDEX_NONE);
}

void UFoliageRTInvokerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PoolManager && bRegistered)
	{
		PoolManager->UnregisterInvoker(GetOwner()->GetActorLocation());
		bRegistered = false;
	}

	Super::EndPlay(EndPlayReason);
}

void UFoliageRTInvokerComponent::TickComponent(float DeltaTime,
                                               ELevelTick TickType,
                                               FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PoolManager) { return; }

	// Drive pool reclaim logic — no separate actor needed.
	PoolManager->Tick(DeltaTime);

	const FVector WorldLoc = GetOwner()->GetActorLocation();

	// ── 1. Cell transition check ──────────────────────────────────────────────
	HandleCellTransition(WorldLoc);

	if (!bRegistered || ActiveSlotIndex == INDEX_NONE) { return; }

	// ── 2. Fetch both RTs and UV for this frame ───────────────────────────────
	UTextureRenderTarget2D* ImpulseRT    = nullptr;
	UTextureRenderTarget2D* ContinuousRT = nullptr;
	FVector2D CellUV;

	if (!PoolManager->GetRTForWorldPosition(WorldLoc, ImpulseRT, ContinuousRT, CellUV))
	{
		return;
	}

	CurrentCellUV = CellUV;

	// ── 3. Velocity ───────────────────────────────────────────────────────────
	const FVector2D RawVelocity = ComputeVelocity(DeltaTime);
	EncodedVelocity = FVector2D(
		EncodeVelocityAxis(RawVelocity.X),
		EncodeVelocityAxis(RawVelocity.Y)
	);

	const FVector2D UVExtent = BrushExtentToUV();

	// ── 4. Continuous RT — smear quad every tick ──────────────────────────────
	if (ContinuousRT && MID_Continuous)
	{
		// Use previous UV as smear start; fall back to current on the first tick.
		const FVector2D SmearStart = (PrevCellUV.X >= 0.f) ? PrevCellUV : CellUV;

		MID_Continuous->SetScalarParameterValue(PN_VelocityX, EncodedVelocity.X);
		MID_Continuous->SetScalarParameterValue(PN_VelocityY, EncodedVelocity.Y);
		MID_Continuous->SetScalarParameterValue(PN_Weight,    BrushWeight);

		DrawContinuousQuad(ContinuousRT, SmearStart, CellUV, UVExtent);
		OnContinuousPaint(ContinuousRT, CellUV, UVExtent);
	}

	// ── 5. Impulse RT — stamp only on new pixel entry ─────────────────────────
	if (ImpulseRT && MID_Impulse && HasMovedToNewPixel(CellUV))
	{
		const float GameTime = GetWorld()->GetTimeSeconds();

		MID_Impulse->SetScalarParameterValue(PN_VelocityX,  EncodedVelocity.X);
		MID_Impulse->SetScalarParameterValue(PN_VelocityY,  EncodedVelocity.Y);
		MID_Impulse->SetScalarParameterValue(PN_Weight,     BrushWeight);
		MID_Impulse->SetScalarParameterValue(PN_ImpactTime, GameTime);

		DrawImpulseStamp(ImpulseRT, CellUV, UVExtent, GameTime);
		OnImpulseStamp(ImpulseRT, CellUV, UVExtent);

		// Record which texel we just stamped so we don't re-stamp next frame.
		const FIntPoint RTSize(
			ImpulseRT->SizeX > 0 ? ImpulseRT->SizeX : 512,
			ImpulseRT->SizeY > 0 ? ImpulseRT->SizeY : 512
		);
		PrevImpulseTexel = FIntPoint(
			FMath::Clamp(FMath::FloorToInt(CellUV.X * RTSize.X), 0, RTSize.X - 1),
			FMath::Clamp(FMath::FloorToInt(CellUV.Y * RTSize.Y), 0, RTSize.Y - 1)
		);
	}

	// ── 6. Advance previous-frame state ──────────────────────────────────────
	PrevWorldLocation = WorldLoc;
	PrevCellUV        = CellUV;
}

// ── Internal helpers ──────────────────────────────────────────────────────────

FVector2D UFoliageRTInvokerComponent::ComputeVelocity(float DeltaTime) const
{
	if (DeltaTime <= KINDA_SMALL_NUMBER)
	{
		return FVector2D::ZeroVector;
	}

	const FVector Delta = GetOwner()->GetActorLocation() - PrevWorldLocation;
	return FVector2D(Delta.X, Delta.Y) / DeltaTime;
}

float UFoliageRTInvokerComponent::EncodeVelocityAxis(float WorldVelAxis) const
{
	// Map [−MaxVelocity..+MaxVelocity] → [0..1].
	// 0.5 = zero.  Clamped so overshoots don't wrap.
	const float Normalized = WorldVelAxis / FMath::Max(MaxVelocity, 1.f);
	return FMath::Clamp(Normalized * 0.5f + 0.5f, 0.f, 1.f);
}

FVector2D UFoliageRTInvokerComponent::BrushExtentToUV() const
{
	// BrushExtent is in world units; CellSize is in world units → ratio = UV extent.
	const float CellSize = PoolManager ? PoolManager->CellSize : 2000.f;
	return FVector2D(
		BrushExtent.X / CellSize,
		BrushExtent.Y / CellSize
	);
}

bool UFoliageRTInvokerComponent::HasMovedToNewPixel(FVector2D CellUV) const
{
	// First-ever tick: PrevImpulseTexel is (-1,-1) — always stamp.
	if (PrevImpulseTexel == FIntPoint(-1, -1)) { return true; }

	const int32 RTRes = PoolManager ? PoolManager->RTResolution : 512;

	const FIntPoint CurrentTexel(
		FMath::Clamp(FMath::FloorToInt(CellUV.X * RTRes), 0, RTRes - 1),
		FMath::Clamp(FMath::FloorToInt(CellUV.Y * RTRes), 0, RTRes - 1)
	);

	return CurrentTexel != PrevImpulseTexel;
}

void UFoliageRTInvokerComponent::DrawContinuousQuad(UTextureRenderTarget2D* RT,
                                                     FVector2D PrevUV,
                                                     FVector2D CurrentUV,
                                                     FVector2D UVExtent)
{
	// UKismetRenderingLibrary::DrawMaterialToRenderTarget paints the MID over the
	// full RT.  The MID itself is responsible for masking to the correct UV region
	// using the UVExtent and CentreUV scalar parameters.
	//
	// For more precise quad-only draws (e.g. via Canvas), override OnContinuousPaint
	// in Blueprint and use DrawMaterial / DrawMaterialSimple on a UCanvas.
	//
	// This base implementation updates the MID centre to the smear midpoint and
	// paints.  The smear is approximated by the material sampling both PrevUV and
	// CurrentUV passed as parameters.

	if (!MID_Continuous || !RT) { return; }

	const FVector2D MidUV = (PrevUV + CurrentUV) * 0.5f;

	MID_Continuous->SetVectorParameterValue(FName("BrushCentreUV"),
		FLinearColor(MidUV.X, MidUV.Y, 0.f, 0.f));
	MID_Continuous->SetVectorParameterValue(FName("BrushPrevUV"),
		FLinearColor(PrevUV.X, PrevUV.Y, 0.f, 0.f));
	MID_Continuous->SetVectorParameterValue(FName("BrushUVExtent"),
		FLinearColor(UVExtent.X, UVExtent.Y, 0.f, 0.f));

	UKismetRenderingLibrary::DrawMaterialToRenderTarget(GetWorld(), RT, MID_Continuous);
}

void UFoliageRTInvokerComponent::DrawImpulseStamp(UTextureRenderTarget2D* RT,
                                                   FVector2D CurrentUV,
                                                   FVector2D UVExtent,
                                                   float GameTime)
{
	if (!MID_Impulse || !RT) { return; }

	MID_Impulse->SetVectorParameterValue(FName("BrushCentreUV"),
		FLinearColor(CurrentUV.X, CurrentUV.Y, 0.f, 0.f));
	MID_Impulse->SetVectorParameterValue(FName("BrushUVExtent"),
		FLinearColor(UVExtent.X, UVExtent.Y, 0.f, 0.f));

	// ImpactTime is also set as a vector so the material can write it to G directly
	// via a custom HLSL node without float precision loss through a scalar.
	MID_Impulse->SetVectorParameterValue(FName("ImpactTimeVec"),
		FLinearColor(GameTime, GameTime, GameTime, GameTime));

	UKismetRenderingLibrary::DrawMaterialToRenderTarget(GetWorld(), RT, MID_Impulse);
}

void UFoliageRTInvokerComponent::HandleCellTransition(const FVector& WorldLocation)
{
	const FIntPoint NewCell = PoolManager->WorldToCell(WorldLocation);

	if (NewCell == CurrentCell) { return; }

	// Unregister from the old cell (starts its decay clock).
	if (bRegistered)
	{
		// Pass a location guaranteed to be inside the old cell.
		const FVector2D OldOrigin = PoolManager->CellToWorldOrigin(CurrentCell);
		const FVector OldCellLoc(
			OldOrigin.X + PoolManager->CellSize * 0.5f,
			OldOrigin.Y + PoolManager->CellSize * 0.5f,
			WorldLocation.Z
		);
		PoolManager->UnregisterInvoker(OldCellLoc);
		bRegistered = false;
	}

	// Register in the new cell.
	ActiveSlotIndex = PoolManager->RegisterInvoker(WorldLocation);
	CurrentCell     = NewCell;
	bRegistered     = (ActiveSlotIndex != INDEX_NONE);

	// Reset smear state — the previous UV is in a different cell's coordinate space.
	PrevCellUV       = FVector2D(-1.f, -1.f);
	PrevImpulseTexel = FIntPoint(-1, -1);

	UE_LOG(LogTemp, Verbose,
		TEXT("[FoliageRTInvoker] Cell transition → (%d,%d)  slot %d"),
		NewCell.X, NewCell.Y, ActiveSlotIndex);
}
