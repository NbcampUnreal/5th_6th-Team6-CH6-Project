#include "Invoker/FoliageRTInvokerComponent.h"

#include "Managers/RTPoolManager.h"
#include "Data/RTBrushTypes.h"
#include "Debug/LogCategory.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

static const FName PN_VelocityX      (TEXT("VelocityX"));
static const FName PN_VelocityY      (TEXT("VelocityY"));
static const FName PN_Weight         (TEXT("Weight"));
static const FName PN_GameTimeFrac   (TEXT("GameTimeFrac"));
static const FName PN_BrushRot       (TEXT("BrushRotation"));
static const FName PN_BrushUVExtent  (TEXT("BrushUVExtent"));

UFoliageRTInvokerComponent::UFoliageRTInvokerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup   = TG_PostUpdateWork;
}

void UFoliageRTInvokerComponent::BeginPlay()
{
    Super::BeginPlay();

    UWorld* World = GetOwner() ? GetOwner()->GetWorld() : nullptr;
    if (!World) { return; }

    PoolManager = World->GetSubsystem<URTPoolManager>();
    if (!PoolManager) { return; }

    if (BrushMaterial_Continuous)
        MID_Continuous = UMaterialInstanceDynamic::Create(BrushMaterial_Continuous, this);

    if (BrushMaterial_Impulse)
        MID_Impulse = UMaterialInstanceDynamic::Create(BrushMaterial_Impulse, this);

    PoolManager->RegisterInvoker(this);

    PrevWorldLocation = GetComponentLocation();
}

void UFoliageRTInvokerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (PoolManager) { PoolManager->UnregisterInvoker(this); }
    Super::EndPlay(EndPlayReason);
}

void UFoliageRTInvokerComponent::TickComponent(float DeltaTime,
                                               ELevelTick TickType,
                                               FActorComponentTickFunction* ThisTickFunction)
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
    const FVector  CompLoc       = GetComponentLocation();
    const float    OwnerYaw      = GetComponentRotation().Yaw;
    const float    GameTime      = GetWorld()->GetTimeSeconds();
    const float    GameTimeFrac  = FMath::Frac(GameTime / TimestampPeriod);
    const float    UVRadius      = BrushRadius / CellSize;
    const FVector2D CellUV       = (FVector2D(CompLoc.X, CompLoc.Y) - CellOriginWS) / CellSize;
    const FLinearColor UVExtentV = FLinearColor(UVRadius, UVRadius, 0.f, 0.f);

    if (MID_Continuous)
    {
        MID_Continuous->SetScalarParameterValue(PN_Weight,        BrushWeight);
        MID_Continuous->SetScalarParameterValue(PN_BrushRot,      OwnerYaw);
        MID_Continuous->SetVectorParameterValue(PN_BrushUVExtent, UVExtentV);
    }

    if (MID_Impulse)
    {
        // Written every tick — SE_BLEND_MAX in draw manager preserves
        // the freshest timestamp per texel. Freezes on invoker exit.
        MID_Impulse->SetScalarParameterValue(PN_VelocityX,    EncodedVelocity.X);
        MID_Impulse->SetScalarParameterValue(PN_VelocityY,    EncodedVelocity.Y);
        MID_Impulse->SetScalarParameterValue(PN_GameTimeFrac, GameTimeFrac);
        MID_Impulse->SetVectorParameterValue(PN_BrushUVExtent, UVExtentV);
    }

    FRTInvokerFrameData Data;
    Data.CellUV          = CellUV;
    Data.UVExtent        = FVector2D(UVRadius, UVRadius);
    Data.EncodedVelocity = EncodedVelocity;
    Data.OwnerYaw        = OwnerYaw;
    Data.GameTimeFrac    = GameTimeFrac;
    Data.SlotIndex       = SlotIndex;
    Data.MID_Continuous  = MID_Continuous;
    Data.MID_Impulse     = MID_Impulse;

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
    const float Normalized = WorldVelAxis / FMath::Max(MaxVelocity, 1.f);
    return FMath::Clamp(Normalized * 0.5f + 0.5f, 0.f, 1.f);
}