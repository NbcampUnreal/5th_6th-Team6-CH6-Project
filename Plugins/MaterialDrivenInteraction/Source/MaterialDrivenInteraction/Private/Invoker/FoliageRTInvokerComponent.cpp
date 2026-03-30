#include "Invoker/FoliageRTInvokerComponent.h"

#include "Managers/RTPoolManager.h"
#include "Data/RTBrushTypes.h"
#include "Data/RTPoolTypes.h"
#include "Debug/LogCategory.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

static const FName PN_Weight        (TEXT("Weight"));
static const FName PN_BrushSoftness (TEXT("Softness"));
static const FName PN_BrushUVExtent (TEXT("BrushUVExtent"));
static const FName PN_VelocityX     (TEXT("VelocityX"));
static const FName PN_VelocityY     (TEXT("VelocityY"));
static const FName PN_DecayRate     (TEXT("DecayRate"));

UFoliageRTInvokerComponent::UFoliageRTInvokerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup   = TG_PostUpdateWork;
}

void UFoliageRTInvokerComponent::BeginPlay()
{
    Super::BeginPlay();

    // Cache shared params from settings
    if (const URTPoolSettings* Settings = GetDefault<URTPoolSettings>())
    {
        CachedMaxVelocity = Settings->MaxVelocity;
        CachedDecayRate   = Settings->DecayRate;
    }

    UWorld* World = GetOwner() ? GetOwner()->GetWorld() : nullptr;
    if (!World) { return; }

    PoolManager = World->GetSubsystem<URTPoolManager>();
    if (!PoolManager) { return; }

    if (BrushMaterial)
        MID_Interaction = UMaterialInstanceDynamic::Create(BrushMaterial, this);

    PoolManager->RegisterInvoker(this);
    PrevWorldLocation = GetComponentLocation();
}

void UFoliageRTInvokerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (PoolManager) { PoolManager->UnregisterInvoker(this); }
    Super::EndPlay(EndPlayReason);
}

void UFoliageRTInvokerComponent::TickComponent(float DeltaTime,
    ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!PoolManager) { return; }
    PoolManager->Tick(DeltaTime);

    if (bFirstTick)
    {
        bFirstTick        = false;
        PrevWorldLocation = GetComponentLocation();
        return;
    }

    if (!PoolManager->IsInvokerActive(this)) { return; }

    const FVector2D RawVelocity = ComputeVelocity(DeltaTime);
    EncodedVelocity = FVector2D(
        EncodeVelocityAxis(RawVelocity.X),
        EncodeVelocityAxis(RawVelocity.Y));

    PrevWorldLocation = GetComponentLocation();
}

FRTInvokerFrameData UFoliageRTInvokerComponent::GetFrameData(
    int32 SlotIndex, FVector2D CellOriginWS, float CellSize) const
{
    const FVector      CompLoc   = GetComponentLocation();
    const float        UVRadius  = BrushRadius / CellSize;
    const FVector2D    CellUV    = (FVector2D(CompLoc.X, CompLoc.Y) - CellOriginWS) / CellSize;
    const FLinearColor UVExtentV = FLinearColor(UVRadius, UVRadius, 0.f, 0.f);

    if (MID_Interaction)
    {
        MID_Interaction->SetScalarParameterValue(PN_Weight,        BrushWeight);
        MID_Interaction->SetScalarParameterValue(PN_BrushSoftness, BrushSoftness);
        MID_Interaction->SetScalarParameterValue(PN_VelocityX,     EncodedVelocity.X);
        MID_Interaction->SetScalarParameterValue(PN_VelocityY,     EncodedVelocity.Y);
        MID_Interaction->SetScalarParameterValue(PN_DecayRate,     CachedDecayRate);
        MID_Interaction->SetVectorParameterValue(PN_BrushUVExtent, UVExtentV);
    }

    FRTInvokerFrameData Data;
    Data.CellUV          = CellUV;
    Data.UVExtent        = FVector2D(UVRadius, UVRadius);
    Data.EncodedVelocity = EncodedVelocity;
    Data.DecayRate       = CachedDecayRate;
    Data.BrushWeight     = BrushWeight;
    Data.BrushSoftness   = BrushSoftness;
    Data.SlotIndex       = SlotIndex;
    Data.MID_Interaction = MID_Interaction;

    return Data;
}

FVector2D UFoliageRTInvokerComponent::ComputeVelocity(float DeltaTime) const
{
    if (DeltaTime <= KINDA_SMALL_NUMBER) { return FVector2D::ZeroVector; }
    const FVector Delta = GetComponentLocation() - PrevWorldLocation;
    return FVector2D(Delta.X, Delta.Y) / DeltaTime;
}

float UFoliageRTInvokerComponent::EncodeVelocityAxis(float WorldVelAxis) const
{
    const float Normalized = WorldVelAxis / FMath::Max(CachedMaxVelocity, 1.f);
    return FMath::Clamp(Normalized * 0.5f + 0.5f, 0.f, 1.f);
}