#include "Invoker/FoliageRTInvokerComponent.h"

#include "Managers/RTPoolManager.h"
#include "Data/RTBrushTypes.h"
#include "Debug/LogCategory.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

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
			TEXT("UFoliageRTInvokerComponent::BeginPlay >> World is null — disabled"));
		return;
	}

	PoolManager = World->GetSubsystem<URTPoolManager>();
	if (!PoolManager)
	{
		UE_LOG(RTFoliageInvoker, Warning,
			TEXT("UFoliageRTInvokerComponent::BeginPlay >> URTPoolManager not found — disabled"));
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

	// Register once — pool manager stores this invoker permanently.
	// Pool evaluation and slot assignment are handled internally each draw cycle.
	PoolManager->RegisterInvoker(this);

	PrevWorldLocation = GetComponentLocation();

	UE_LOG(RTFoliageInvoker, Log,
		TEXT("UFoliageRTInvokerComponent::BeginPlay >> Registered  Loc(%.0f,%.0f,%.0f)"),
		PrevWorldLocation.X, PrevWorldLocation.Y, PrevWorldLocation.Z);
}

void UFoliageRTInvokerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PoolManager) { PoolManager->UnregisterInvoker(this); }

	PrevImpulseTexelMap.Empty();

	const FVector Loc = GetComponentLocation();
	UE_LOG(RTFoliageInvoker, Log,
		TEXT("UFoliageRTInvokerComponent::EndPlay >> Unregistered  Loc(%.0f,%.0f,%.0f)"),
		Loc.X, Loc.Y, Loc.Z);

	Super::EndPlay(EndPlayReason);
}

void UFoliageRTInvokerComponent::TickComponent(float DeltaTime,
                                               ELevelTick TickType,
                                               FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PoolManager) { return; }

	// Frame guard — prevents duplicate pool work per frame
	PoolManager->Tick(DeltaTime);

	if (bFirstTick)
	{
		bFirstTick        = false;
		PrevWorldLocation = GetComponentLocation();
		return;
	}

	// Update velocity each tick — draw manager reads it via GetFrameData()
	const FVector2D RawVelocity = ComputeVelocity(DeltaTime);
	EncodedVelocity = FVector2D(
		EncodeVelocityAxis(RawVelocity.X),
		EncodeVelocityAxis(RawVelocity.Y)
	);

	PrevWorldLocation = GetComponentLocation();
}

FRTInvokerFrameData UFoliageRTInvokerComponent::GetFrameData(
	int32 SlotIndex, FVector2D CellOriginWS, float CellSize) const
{
	const FVector CompLoc  = GetComponentLocation();
	const float   OwnerYaw = GetComponentRotation().Yaw;
	const float   GameTime = GetWorld()->GetTimeSeconds();
	const float   UVRadius = BrushRadius / CellSize;

	// Brush centre in RT UV space — not clamped, DrawTiledBrush handles wraparound
	const FVector2D CellUV = (FVector2D(CompLoc.X, CompLoc.Y) - CellOriginWS) / CellSize;

	// Cell index for texel map lookup
	const FIntPoint CellIdx(
		FMath::FloorToInt(CellOriginWS.X / CellSize),
		FMath::FloorToInt(CellOriginWS.Y / CellSize)
	);

	const FIntPoint* PrevTexelPtr = PrevImpulseTexelMap.Find(CellIdx);
	const FIntPoint  PrevTexel    = PrevTexelPtr ? *PrevTexelPtr : FIntPoint(-1, -1);
	const bool       bShouldStamp = HasMovedToNewPixel(CellUV, PrevTexel);

	// Set MID parameters — BrushCentreUV is set per tile in DrawTiledBrush
	if (MID_Continuous)
	{
		MID_Continuous->SetScalarParameterValue(PN_Weight,   BrushWeight);
		MID_Continuous->SetScalarParameterValue(PN_BrushRot, OwnerYaw);
		MID_Continuous->SetVectorParameterValue(FName("BrushUVExtent"),
			FLinearColor(UVRadius, UVRadius, 0.f, 0.f));
	}

	if (MID_Impulse && bShouldStamp)
	{
		MID_Impulse->SetScalarParameterValue(PN_VelocityX,  EncodedVelocity.X);
		MID_Impulse->SetScalarParameterValue(PN_VelocityY,  EncodedVelocity.Y);
		MID_Impulse->SetScalarParameterValue(PN_ImpactTime, GameTime);
		MID_Impulse->SetVectorParameterValue(FName("BrushUVExtent"),
			FLinearColor(UVRadius, UVRadius, 0.f, 0.f));
	}

	// Update texel tracking
	if (bShouldStamp)
	{
		const int32 RTRes = PoolManager ? PoolManager->RTResolution : 512;
		PrevImpulseTexelMap.Add(CellIdx, FIntPoint(
			FMath::Clamp(FMath::FloorToInt(CellUV.X * RTRes), 0, RTRes - 1),
			FMath::Clamp(FMath::FloorToInt(CellUV.Y * RTRes), 0, RTRes - 1)
		));

		UE_LOG(RTFoliageInvoker, Verbose,
			TEXT("UFoliageRTInvokerComponent::GetFrameData >> Stamp — UV(%.3f,%.3f) t=%.2f"),
			CellUV.X, CellUV.Y, GameTime);
	}

	FRTInvokerFrameData Data;
	Data.CellUV          = CellUV;
	Data.UVExtent        = FVector2D(UVRadius, UVRadius);
	Data.EncodedVelocity = EncodedVelocity;
	Data.OwnerYaw        = OwnerYaw;
	Data.GameTime        = GameTime;
	Data.SlotIndex       = SlotIndex;
	Data.bShouldStamp    = bShouldStamp;
	Data.MID_Continuous  = MID_Continuous;
	Data.MID_Impulse     = MID_Impulse;

	return Data;
}

// ── Internal helpers ──────────────────────────────────────────────────────────

FVector2D UFoliageRTInvokerComponent::ComputeVelocity(float DeltaTime) const
{
	if (DeltaTime <= KINDA_SMALL_NUMBER) { return FVector2D::ZeroVector; }
	const FVector Delta = GetComponentLocation() - PrevWorldLocation;
	return FVector2D(Delta.X, Delta.Y) / DeltaTime;
}

float UFoliageRTInvokerComponent::EncodeVelocityAxis(float WorldVelAxis) const
{
	const float Normalized = WorldVelAxis / FMath::Max(MaxVelocity, 1.f);
	return FMath::Clamp(Normalized * 0.5f + 0.5f, 0.f, 1.f);
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