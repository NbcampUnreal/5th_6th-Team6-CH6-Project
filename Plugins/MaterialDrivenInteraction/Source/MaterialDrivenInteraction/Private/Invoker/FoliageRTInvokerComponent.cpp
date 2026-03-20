#include "Invoker/FoliageRTInvokerComponent.h"

#include "PoolManager/RTPoolManager.h"
#include "Debug/LogCategory.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

static const FName PN_VelocityX  (TEXT("VelocityX"));
static const FName PN_VelocityY  (TEXT("VelocityY"));
static const FName PN_Weight     (TEXT("Weight"));
static const FName PN_ImpactTime (TEXT("ImpactTime"));
static const FName PN_BrushRot   (TEXT("BrushRotation"));

UFoliageRTInvokerComponent::UFoliageRTInvokerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup   = TG_PostUpdateWork;
}

void UFoliageRTInvokerComponent::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetOwner() ? GetOwner()->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(RTFoliageInvoker, Warning,
			TEXT("UFoliageRTInvokerComponent::BeginPlay >> World is null — component disabled"));
		return;
	}

	PoolManager = World->GetSubsystem<URTPoolManager>();
	if (!PoolManager)
	{
		UE_LOG(RTFoliageInvoker, Warning,
			TEXT("UFoliageRTInvokerComponent::BeginPlay >> URTPoolManager subsystem not found — component disabled"));
		return;
	}

	if (BrushMaterial_Continuous)
	{
		MID_Continuous = UMaterialInstanceDynamic::Create(BrushMaterial_Continuous, this);
		UE_LOG(RTFoliageInvoker, Log,
			TEXT("UFoliageRTInvokerComponent::BeginPlay >> MID_Continuous created from '%s'"),
			*BrushMaterial_Continuous->GetName());
	}
	else
	{
		UE_LOG(RTFoliageInvoker, Warning,
			TEXT("UFoliageRTInvokerComponent::BeginPlay >> BrushMaterial_Continuous not assigned"));
	}

	if (BrushMaterial_Impulse)
	{
		MID_Impulse = UMaterialInstanceDynamic::Create(BrushMaterial_Impulse, this);
		UE_LOG(RTFoliageInvoker, Log,
			TEXT("UFoliageRTInvokerComponent::BeginPlay >> MID_Impulse created from '%s'"),
			*BrushMaterial_Impulse->GetName());
	}
	else
	{
		UE_LOG(RTFoliageInvoker, Warning,
			TEXT("UFoliageRTInvokerComponent::BeginPlay >> BrushMaterial_Impulse not assigned"));
	}

	PrevWorldLocation = GetOwner()->GetActorLocation();

	UE_LOG(RTFoliageInvoker, Log,
		TEXT("UFoliageRTInvokerComponent::BeginPlay >> Ready  OwnerLoc(%.0f,%.0f,%.0f)"),
		PrevWorldLocation.X, PrevWorldLocation.Y, PrevWorldLocation.Z);
}

void UFoliageRTInvokerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PoolManager)
	{
		for (const FIntPoint& Cell : CurrentCells)
		{
			const FVector2D Origin = PoolManager->CellToWorldOrigin(Cell);
			PoolManager->UnregisterInvoker(FVector(
				Origin.X + PoolManager->CellSize * 0.5f,
				Origin.Y + PoolManager->CellSize * 0.5f,
				GetOwner() ? GetOwner()->GetActorLocation().Z : 0.f));
		}
	}

	const int32 CellCount = CurrentCells.Num();
	CurrentCells.Empty();
	ActiveSlotIndices.Empty();
	PrevImpulseTexelMap.Empty();

	const FVector OwnerLoc = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
	UE_LOG(RTFoliageInvoker, Log,
		TEXT("UFoliageRTInvokerComponent::EndPlay >> Unregistered from %d cells  OwnerLoc(%.0f,%.0f,%.0f)"),
		CellCount, OwnerLoc.X, OwnerLoc.Y, OwnerLoc.Z);

	Super::EndPlay(EndPlayReason);
}

void UFoliageRTInvokerComponent::TickComponent(float DeltaTime,
                                               ELevelTick TickType,
                                               FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PoolManager) { return; }

	// Drives per-frame ContinuousRT clear and slot reclaim.
	// Frame-guarded in RTPoolManager — safe to call from multiple invokers.
	PoolManager->Tick(DeltaTime);

	const FVector WorldLoc = GetOwner()->GetActorLocation();
	const float   OwnerYaw = GetOwner()->GetActorRotation().Yaw;

	// ── 0. First tick — seed state only, no drawing ───────────────────────────
	if (bFirstTick)
	{
		bFirstTick        = false;
		PrevWorldLocation = WorldLoc;
		UpdateCellRegistrations(WorldLoc);

		UE_LOG(RTFoliageInvoker, Verbose,
			TEXT("UFoliageRTInvokerComponent::TickComponent >> First tick — registered %d cells  OwnerLoc(%.0f,%.0f,%.0f)"),
			CurrentCells.Num(), WorldLoc.X, WorldLoc.Y, WorldLoc.Z);
		return;
	}

	// ── 1. Update cell registrations based on brush radius ────────────────────
	UpdateCellRegistrations(WorldLoc);

	if (CurrentCells.Num() == 0) { return; }

	// ── 2. Velocity — used by impulse stamp only ──────────────────────────────
	const FVector2D RawVelocity = ComputeVelocity(DeltaTime);
	EncodedVelocity = FVector2D(
		EncodeVelocityAxis(RawVelocity.X),
		EncodeVelocityAxis(RawVelocity.Y)
	);

	const FVector2D UVExtent = BrushExtentToUV();

	// ── 3. Draw into each overlapping cell ────────────────────────────────────
	for (int32 i = 0; i < CurrentCells.Num(); ++i)
	{
		const FIntPoint Cell    = CurrentCells[i];
		const int32     SlotIdx = ActiveSlotIndices[i];

		if (SlotIdx == INDEX_NONE) { continue; }

		// Compute UV of brush centre within this cell
		const FVector2D CellOrigin = PoolManager->CellToWorldOrigin(Cell);
		FVector2D CellUV = (FVector2D(WorldLoc.X, WorldLoc.Y) - CellOrigin) / PoolManager->CellSize;
		CellUV = CellUV.ClampAxes(0.f, 1.f);

		UTextureRenderTarget2D* ImpulseRT    = nullptr;
		UTextureRenderTarget2D* ContinuousRT = nullptr;
		FVector2D DummyOrigin;
		if (!PoolManager->GetSlotData(SlotIdx, ImpulseRT, ContinuousRT, DummyOrigin))
		{
			continue;
		}

		CurrentCellUV = CellUV;

		FIntPoint& PrevTexel = PrevImpulseTexelMap.FindOrAdd(Cell, FIntPoint(-1, -1));

		// ── Continuous RT ─────────────────────────────────────────────────────
		// RTPoolManager::Tick already cleared this RT to neutral (0.5,0.5,0,0)
		// once at the start of this frame. Draw directly — no clear here.
		// Multiple invokers in the same cell accumulate additively.
		if (ContinuousRT && MID_Continuous)
		{
			DrawContinuousQuad(ContinuousRT, CellUV, UVExtent, OwnerYaw);
			OnContinuousPaint(ContinuousRT, CellUV, UVExtent);
		}

		// ── Impulse RT ────────────────────────────────────────────────────────
		// Stamps GameTime into G and velocity into RB on new pixel entry.
		// Re-triggers FRT_DampedSine whenever the invoker moves out of and
		// back into a texel — PrevImpulseTexelMap tracks the last stamped texel.
		if (ImpulseRT && MID_Impulse && HasMovedToNewPixel(CellUV, PrevTexel))
		{
			const float GameTime = GetWorld()->GetTimeSeconds();

			MID_Impulse->SetScalarParameterValue(PN_VelocityX,  EncodedVelocity.X);
			MID_Impulse->SetScalarParameterValue(PN_VelocityY,  EncodedVelocity.Y);
			MID_Impulse->SetScalarParameterValue(PN_Weight,     BrushWeight);
			MID_Impulse->SetScalarParameterValue(PN_ImpactTime, GameTime);

			DrawImpulseStamp(ImpulseRT, CellUV, UVExtent, GameTime);
			OnImpulseStamp(ImpulseRT, CellUV, UVExtent);

			const FIntPoint RTSize(
				ImpulseRT->SizeX > 0 ? ImpulseRT->SizeX : 512,
				ImpulseRT->SizeY > 0 ? ImpulseRT->SizeY : 512
			);
			PrevTexel = FIntPoint(
				FMath::Clamp(FMath::FloorToInt(CellUV.X * RTSize.X), 0, RTSize.X - 1),
				FMath::Clamp(FMath::FloorToInt(CellUV.Y * RTSize.Y), 0, RTSize.Y - 1)
			);

			UE_LOG(RTFoliageInvoker, Verbose,
				TEXT("UFoliageRTInvokerComponent::TickComponent >> Impulse stamped — Cell(%d,%d) UV(%.3f,%.3f) Vel(%.1f,%.1f) t=%.2f  OwnerLoc(%.0f,%.0f,%.0f)"),
				Cell.X, Cell.Y, CellUV.X, CellUV.Y,
				RawVelocity.X, RawVelocity.Y, GameTime,
				WorldLoc.X, WorldLoc.Y, WorldLoc.Z);
		}
	}

	// ── 4. Advance state ──────────────────────────────────────────────────────
	PrevWorldLocation = WorldLoc;
}

// ── Internal helpers ──────────────────────────────────────────────────────────

TArray<FIntPoint> UFoliageRTInvokerComponent::GetOverlappingCells(const FVector& WorldLocation) const
{
	TArray<FIntPoint> Result;
	if (!PoolManager) { return Result; }

	const FVector2D Center(WorldLocation.X, WorldLocation.Y);
	const FVector2D Min = Center - BrushExtent;
	const FVector2D Max = Center + BrushExtent;

	const FIntPoint MinCell = PoolManager->WorldToCell(FVector(Min.X, Min.Y, 0.f));
	const FIntPoint MaxCell = PoolManager->WorldToCell(FVector(Max.X, Max.Y, 0.f));

	for (int32 X = MinCell.X; X <= MaxCell.X; ++X)
	{
		for (int32 Y = MinCell.Y; Y <= MaxCell.Y; ++Y)
		{
			Result.Add(FIntPoint(X, Y));
		}
	}

	return Result;
}

void UFoliageRTInvokerComponent::UpdateCellRegistrations(const FVector& WorldLocation)
{
	const TArray<FIntPoint> NewCells = GetOverlappingCells(WorldLocation);

	for (int32 i = CurrentCells.Num() - 1; i >= 0; --i)
	{
		const FIntPoint Cell = CurrentCells[i];
		if (!NewCells.Contains(Cell))
		{
			const FVector2D Origin = PoolManager->CellToWorldOrigin(Cell);
			PoolManager->UnregisterInvoker(FVector(
				Origin.X + PoolManager->CellSize * 0.5f,
				Origin.Y + PoolManager->CellSize * 0.5f,
				WorldLocation.Z));

			PrevImpulseTexelMap.Remove(Cell);
			CurrentCells.RemoveAt(i);
			ActiveSlotIndices.RemoveAt(i);

			UE_LOG(RTFoliageInvoker, Log,
				TEXT("UFoliageRTInvokerComponent::UpdateCellRegistrations >> Left Cell (%d,%d)  OwnerLoc(%.0f,%.0f,%.0f)"),
				Cell.X, Cell.Y, WorldLocation.X, WorldLocation.Y, WorldLocation.Z);
		}
	}

	for (const FIntPoint& Cell : NewCells)
	{
		if (!CurrentCells.Contains(Cell))
		{
			const FVector2D Origin  = PoolManager->CellToWorldOrigin(Cell);
			const FVector   CellLoc = FVector(
				Origin.X + PoolManager->CellSize * 0.5f,
				Origin.Y + PoolManager->CellSize * 0.5f,
				WorldLocation.Z);

			const int32 SlotIdx = PoolManager->RegisterInvoker(CellLoc);
			CurrentCells.Add(Cell);
			ActiveSlotIndices.Add(SlotIdx);

			UE_LOG(RTFoliageInvoker, Log,
				TEXT("UFoliageRTInvokerComponent::UpdateCellRegistrations >> Entered Cell (%d,%d) Slot %d  OwnerLoc(%.0f,%.0f,%.0f)"),
				Cell.X, Cell.Y, SlotIdx,
				WorldLocation.X, WorldLocation.Y, WorldLocation.Z);
		}
	}
}

FVector2D UFoliageRTInvokerComponent::ComputeVelocity(float DeltaTime) const
{
	if (DeltaTime <= KINDA_SMALL_NUMBER) { return FVector2D::ZeroVector; }
	const FVector Delta = GetOwner()->GetActorLocation() - PrevWorldLocation;
	return FVector2D(Delta.X, Delta.Y) / DeltaTime;
}

float UFoliageRTInvokerComponent::EncodeVelocityAxis(float WorldVelAxis) const
{
	const float Normalized = WorldVelAxis / FMath::Max(MaxVelocity, 1.f);
	return FMath::Clamp(Normalized * 0.5f + 0.5f, 0.f, 1.f);
}

FVector2D UFoliageRTInvokerComponent::BrushExtentToUV() const
{
	const float CellSize = PoolManager ? PoolManager->CellSize : 2000.f;
	return FVector2D(BrushExtent.X / CellSize, BrushExtent.Y / CellSize);
}

bool UFoliageRTInvokerComponent::HasMovedToNewPixel(FVector2D CellUV, const FIntPoint& PrevTexel) const
{
	const int32 RTRes = PoolManager ? PoolManager->RTResolution : 512;

	const FIntPoint CurrentTexel(
		FMath::Clamp(FMath::FloorToInt(CellUV.X * RTRes), 0, RTRes - 1),
		FMath::Clamp(FMath::FloorToInt(CellUV.Y * RTRes), 0, RTRes - 1)
	);

	return CurrentTexel != PrevTexel;
}

void UFoliageRTInvokerComponent::DrawContinuousQuad(UTextureRenderTarget2D* RT,
                                                     FVector2D CurrentUV,
                                                     FVector2D UVExtent,
                                                     float OwnerYaw)
{
	if (!MID_Continuous || !RT) { return; }

	MID_Continuous->SetScalarParameterValue(PN_Weight,   BrushWeight);
	MID_Continuous->SetScalarParameterValue(PN_BrushRot, OwnerYaw);
	MID_Continuous->SetVectorParameterValue(FName("BrushCentreUV"),
		FLinearColor(CurrentUV.X, CurrentUV.Y, 0.f, 0.f));
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
	MID_Impulse->SetScalarParameterValue(FName("ImpactTime"),GameTime);

	UKismetRenderingLibrary::DrawMaterialToRenderTarget(GetWorld(), RT, MID_Impulse);
}