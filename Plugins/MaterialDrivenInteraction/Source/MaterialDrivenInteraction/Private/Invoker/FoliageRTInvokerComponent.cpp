
#include "Invoker/FoliageRTInvokerComponent.h"

#include "PoolManager/RTPoolManager.h"
#include "Debug/LogCategory.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

// ── Parameter name constants ──────────────────────────────────────────────────

static const FName PN_VelocityX  (TEXT("VelocityX"));
static const FName PN_VelocityY  (TEXT("VelocityY"));
static const FName PN_Weight     (TEXT("Weight"));
static const FName PN_ImpactTime (TEXT("ImpactTime"));

// ── Constructor ───────────────────────────────────────────────────────────────

UFoliageRTInvokerComponent::UFoliageRTInvokerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup   = TG_PostUpdateWork;
}

// ── UActorComponent ───────────────────────────────────────────────────────────

void UFoliageRTInvokerComponent::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World) { return; }

	PoolManager = World->GetSubsystem<URTPoolManager>();
	if (!PoolManager)
	{
		UE_LOG(RTFoliageInvoker, Warning,
			TEXT("UFoliageRTInvokerComponent::BeginPlay >> URTPoolManager subsystem not found"));
		return;
	}

	if (BrushMaterial_Continuous)
	{
		MID_Continuous = UMaterialInstanceDynamic::Create(BrushMaterial_Continuous, this);
	}
	else
	{
		UE_LOG(RTFoliageInvoker, Warning,
			TEXT("UFoliageRTInvokerComponent::BeginPlay >> BrushMaterial_Continuous is not assigned"));
	}

	if (BrushMaterial_Impulse)
	{
		MID_Impulse = UMaterialInstanceDynamic::Create(BrushMaterial_Impulse, this);
	}
	else
	{
		UE_LOG(RTFoliageInvoker, Warning,
			TEXT("UFoliageRTInvokerComponent::BeginPlay >> BrushMaterial_Impulse is not assigned"));
	}

	PrevWorldLocation = GetOwner()->GetActorLocation();

	const FVector StartLoc = GetOwner()->GetActorLocation();
	ActiveSlotIndex = PoolManager->RegisterInvoker(StartLoc);
	CurrentCell     = PoolManager->WorldToCell(StartLoc);
	bRegistered     = (ActiveSlotIndex != INDEX_NONE);

	UE_LOG(RTFoliageInvoker, Log,
		TEXT("UFoliageRTInvokerComponent::BeginPlay >> Registered at cell (%d,%d) slot %d"),
		CurrentCell.X, CurrentCell.Y, ActiveSlotIndex);
}

void UFoliageRTInvokerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PoolManager && bRegistered)
	{
		PoolManager->UnregisterInvoker(GetOwner()->GetActorLocation());
		bRegistered = false;

		UE_LOG(RTFoliageInvoker, Log,
			TEXT("UFoliageRTInvokerComponent::EndPlay >> Unregistered from cell (%d,%d)"),
			CurrentCell.X, CurrentCell.Y);
	}

	Super::EndPlay(EndPlayReason);
}

void UFoliageRTInvokerComponent::TickComponent(float DeltaTime,
                                               ELevelTick TickType,
                                               FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PoolManager) { return; }

	PoolManager->Tick(DeltaTime);

	const FVector WorldLoc = GetOwner()->GetActorLocation();

	// ── 0. First tick — seed state only, do not draw ──────────────────────────
	// Prevents a G=0 impulse stamp on spawn before the pawn has moved.
	if (bFirstTick)
	{
		bFirstTick        = false;
		PrevWorldLocation = WorldLoc;
		HandleCellTransition(WorldLoc);

		if (bRegistered && ActiveSlotIndex != INDEX_NONE)
		{
			UTextureRenderTarget2D* DummyImpulse    = nullptr;
			UTextureRenderTarget2D* DummyContinuous = nullptr;
			FVector2D SeedUV;

			if (PoolManager->GetRTForWorldPosition(WorldLoc, DummyImpulse, DummyContinuous, SeedUV))
			{
				PrevCellUV = SeedUV;
				const int32 RTRes = PoolManager->RTResolution;
				PrevImpulseTexel  = FIntPoint(
					FMath::Clamp(FMath::FloorToInt(SeedUV.X * RTRes), 0, RTRes - 1),
					FMath::Clamp(FMath::FloorToInt(SeedUV.Y * RTRes), 0, RTRes - 1)
				);
			}

			UE_LOG(RTFoliageInvoker, Verbose,
				TEXT("UFoliageRTInvokerComponent::TickComponent >> First tick seeded — UV (%.3f,%.3f) texel (%d,%d)"),
				PrevCellUV.X, PrevCellUV.Y, PrevImpulseTexel.X, PrevImpulseTexel.Y);
		}
		return;
	}

	// ── 1. Cell transition check ──────────────────────────────────────────────
	HandleCellTransition(WorldLoc);

	if (!bRegistered || ActiveSlotIndex == INDEX_NONE) { return; }

	// ── 2. Fetch RTs and UV ───────────────────────────────────────────────────
	UTextureRenderTarget2D* ImpulseRT    = nullptr;
	UTextureRenderTarget2D* ContinuousRT = nullptr;
	FVector2D CellUV;

	if (!PoolManager->GetRTForWorldPosition(WorldLoc, ImpulseRT, ContinuousRT, CellUV))
	{
		UE_LOG(RTFoliageInvoker, Verbose,
			TEXT("UFoliageRTInvokerComponent::TickComponent >> GetRTForWorldPosition returned false at (%.0f,%.0f)"),
			WorldLoc.X, WorldLoc.Y);
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

		const FIntPoint RTSize(
			ImpulseRT->SizeX > 0 ? ImpulseRT->SizeX : 512,
			ImpulseRT->SizeY > 0 ? ImpulseRT->SizeY : 512
		);
		PrevImpulseTexel = FIntPoint(
			FMath::Clamp(FMath::FloorToInt(CellUV.X * RTSize.X), 0, RTSize.X - 1),
			FMath::Clamp(FMath::FloorToInt(CellUV.Y * RTSize.Y), 0, RTSize.Y - 1)
		);

		UE_LOG(RTFoliageInvoker, Verbose,
			TEXT("UFoliageRTInvokerComponent::TickComponent >> Impulse stamped — UV (%.3f,%.3f) vel (%.1f,%.1f) t=%.2f"),
			CellUV.X, CellUV.Y, RawVelocity.X, RawVelocity.Y, GameTime);
	}

	// ── 6. Advance state ──────────────────────────────────────────────────────
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
	const float Normalized = WorldVelAxis / FMath::Max(MaxVelocity, 1.f);
	return FMath::Clamp(Normalized * 0.5f + 0.5f, 0.f, 1.f);
}

FVector2D UFoliageRTInvokerComponent::BrushExtentToUV() const
{
	const float CellSize = PoolManager ? PoolManager->CellSize : 2000.f;
	return FVector2D(
		BrushExtent.X / CellSize,
		BrushExtent.Y / CellSize
	);
}

bool UFoliageRTInvokerComponent::HasMovedToNewPixel(FVector2D CellUV) const
{
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
	MID_Impulse->SetVectorParameterValue(FName("ImpactTimeVec"),
		FLinearColor(GameTime, GameTime, GameTime, GameTime));

	UKismetRenderingLibrary::DrawMaterialToRenderTarget(GetWorld(), RT, MID_Impulse);
}

void UFoliageRTInvokerComponent::HandleCellTransition(const FVector& WorldLocation)
{
	const FIntPoint NewCell = PoolManager->WorldToCell(WorldLocation);

	if (NewCell == CurrentCell) { return; }

	if (bRegistered)
	{
		const FVector2D OldOrigin = PoolManager->CellToWorldOrigin(CurrentCell);
		const FVector OldCellLoc(
			OldOrigin.X + PoolManager->CellSize * 0.5f,
			OldOrigin.Y + PoolManager->CellSize * 0.5f,
			WorldLocation.Z
		);
		PoolManager->UnregisterInvoker(OldCellLoc);
		bRegistered = false;

		UE_LOG(RTFoliageInvoker, Log,
			TEXT("UFoliageRTInvokerComponent::HandleCellTransition >> Left cell (%d,%d)"),
			CurrentCell.X, CurrentCell.Y);
	}

	ActiveSlotIndex = PoolManager->RegisterInvoker(WorldLocation);
	CurrentCell     = NewCell;
	bRegistered     = (ActiveSlotIndex != INDEX_NONE);

	// Reset smear state — previous UV belongs to the old cell's coordinate space
	PrevCellUV       = FVector2D(-1.f, -1.f);
	PrevImpulseTexel = FIntPoint(-1, -1);

	UE_LOG(RTFoliageInvoker, Log,
		TEXT("UFoliageRTInvokerComponent::HandleCellTransition >> Entered cell (%d,%d) slot %d"),
		NewCell.X, NewCell.Y, ActiveSlotIndex);
}
