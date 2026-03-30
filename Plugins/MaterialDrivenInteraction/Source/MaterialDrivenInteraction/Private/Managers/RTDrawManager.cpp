#include "Managers/RTDrawManager.h"

#include "Managers/RTPoolManager.h"
#include "Invoker/FoliageRTInvokerComponent.h"
#include "Data/RTPoolTypes.h"
#include "Data/RTBrushTypes.h"
#include "Debug/LogCategory.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "Engine/World.h"


static const FLinearColor GInteractionNeutral(
    32896.0f / 65535.0f,
    32896.0f / 65535.0f,
    0.0f, 0.0f);

bool URTDrawManager::ShouldCreateSubsystem(UObject* Outer) const
{
    UWorld* World = Cast<UWorld>(Outer);
    return World && (World->IsGameWorld() || World->IsPlayInEditor());
}

void URTDrawManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Collection.InitializeDependency<URTPoolManager>();
    Super::Initialize(Collection);

    PoolManager = GetWorld()->GetSubsystem<URTPoolManager>();
    if (!PoolManager) { return; }

    // Load decay material from settings and create MID
    if (const URTPoolSettings* Settings = GetDefault<URTPoolSettings>())
    {
        UMaterialInterface* DecayMat = Settings->DecayMaterial.LoadSynchronous();
        if (DecayMat)
            DecayMID = UMaterialInstanceDynamic::Create(DecayMat, this);
    }

    const TArray<FRTPoolEntry>& Pool = PoolManager->GetPool();
    for (int32 i = 0; i < Pool.Num(); ++i)
    {
        if (Pool[i].InteractionRT)
            UKismetRenderingLibrary::ClearRenderTarget2D(
                GetWorld(), Pool[i].InteractionRT, GInteractionNeutral);
    }

    UE_LOG(RTFoliageInvoker, Log, TEXT("URTDrawManager::Initialize >> Ready"));
}

void URTDrawManager::Deinitialize()
{
    Super::Deinitialize();
}

void URTDrawManager::Update(float DeltaTime)
{
    if (!PoolManager || !GetWorld()) { return; }

    const TArray<TTuple<UFoliageRTInvokerComponent*, int32>> AssignedInvokers =
        PoolManager->EvaluateAndAssignSlots();

    ProcessPendingClears();
    DrawAllInvokers(AssignedInvokers);
    PoolManager->ReclaimExpiredSlots();
}

void URTDrawManager::DrawAllInvokers(
    const TArray<TTuple<UFoliageRTInvokerComponent*, int32>>& AssignedInvokers) const
{
    if (AssignedInvokers.Num() == 0) { return; }

    for (int32 SlotIdx = 0; SlotIdx < PoolManager->PoolSize; ++SlotIdx)
    {
        UTextureRenderTarget2D* InteractionRT = nullptr;
        FVector2D               DummyOrigin;

        if (!PoolManager->GetSlotData(SlotIdx, InteractionRT, DummyOrigin)) { continue; }
        if (InteractionRT)
            DrawSlot(InteractionRT, SlotIdx, AssignedInvokers);
    }
}

void URTDrawManager::DrawSlot(UTextureRenderTarget2D* RT, int32 SlotIdx,
    const TArray<TTuple<UFoliageRTInvokerComponent*, int32>>& AssignedInvokers) const
{
    const TArray<FRTPoolEntry>& Pool = PoolManager->GetPool();

    // Clear pass — reset directions and height, preserve progression
    if (DecayMID && Pool.IsValidIndex(SlotIdx))
    {
        UCanvas* Canvas = nullptr;
        FVector2D CanvasSize;
        FDrawToRenderTargetContext Context;

        UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(GetWorld(), RT, Canvas, CanvasSize, Context);
        if (Canvas)
        {
            DecayMID->SetTextureParameterValue(FName("PreviousRT"), RT);
            DecayMID->SetScalarParameterValue(FName("Mode"), 0.0f);

            FCanvasTileItem TileItem(FVector2D::ZeroVector, DecayMID->GetRenderProxy(), CanvasSize);
            TileItem.UV0       = FVector2D(0.f, 0.f);
            TileItem.UV1       = FVector2D(1.f, 1.f);
            TileItem.BlendMode = SE_BLEND_Opaque;
            Canvas->DrawItem(TileItem);

            UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);
        }
    }

    // Brush pass — accumulate all invokers into cleared RT
    {
        UCanvas* Canvas = nullptr;
        FVector2D CanvasSize;
        FDrawToRenderTargetContext Context;

        UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(GetWorld(), RT, Canvas, CanvasSize, Context);
        if (!Canvas) { return; }

        for (const TTuple<UFoliageRTInvokerComponent*, int32>& Pair : AssignedInvokers)
        {
            if (Pair.Get<1>() != SlotIdx) { continue; }

            UFoliageRTInvokerComponent* Invoker = Pair.Get<0>();
            if (!Invoker) { continue; }

            FVector2D CellOriginWS;
            UTextureRenderTarget2D* Dummy = nullptr;
            PoolManager->GetSlotData(SlotIdx, Dummy, CellOriginWS);

            const FRTInvokerFrameData Data =
                Invoker->GetFrameData(SlotIdx, CellOriginWS, PoolManager->CellSize);

            if (!Data.MID_Interaction) { continue; }

            Data.MID_Interaction->SetTextureParameterValue(FName("PreviousRT"), RT);
            DrawBrush(Canvas, CanvasSize, Data.MID_Interaction, Data.CellUV, Data.UVExtent);
        }

        UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);
    }

    // Decay pass — decay progression after brushes drawn
    if (DecayMID && Pool.IsValidIndex(SlotIdx))
    {
        UCanvas* Canvas = nullptr;
        FVector2D CanvasSize;
        FDrawToRenderTargetContext Context;

        UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(GetWorld(), RT, Canvas, CanvasSize, Context);
        if (Canvas)
        {
            DecayMID->SetTextureParameterValue(FName("PreviousRT"), RT);
            DecayMID->SetScalarParameterValue(FName("Mode"), 1.0f);
            DecayMID->SetScalarParameterValue(FName("DecayRate"), Pool[SlotIdx].LastDecayRate);

            FCanvasTileItem TileItem(FVector2D::ZeroVector, DecayMID->GetRenderProxy(), CanvasSize);
            TileItem.UV0       = FVector2D(0.f, 0.f);
            TileItem.UV1       = FVector2D(1.f, 1.f);
            TileItem.BlendMode = SE_BLEND_Opaque;
            Canvas->DrawItem(TileItem);

            UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);
        }
    }
}

void URTDrawManager::DrawBrush(
    UCanvas* Canvas,
    FVector2D CanvasSize,
    UMaterialInstanceDynamic* MID,
    FVector2D CentreUV,
    FVector2D UVExtent) const
{
    if (!MID || !Canvas) { return; }

    MID->SetVectorParameterValue(
        FName("BrushCentreUV"),
        FLinearColor(CentreUV.X, CentreUV.Y, 0.f, 0.f));

    MID->SetVectorParameterValue(
        FName("BrushUVExtent"),
        FLinearColor(UVExtent.X, UVExtent.Y, 0.f, 0.f));

    FCanvasTileItem TileItem(FVector2D::ZeroVector, MID->GetRenderProxy(), CanvasSize);
    TileItem.UV0       = FVector2D(0.f, 0.f);
    TileItem.UV1       = FVector2D(1.f, 1.f);
    TileItem.BlendMode = SE_BLEND_Opaque;
    Canvas->DrawItem(TileItem);
}

void URTDrawManager::ProcessPendingClears() const
{
    const TArray<FRTPoolEntry>& Pool = PoolManager->GetPool();
    for (int32 i = 0; i < Pool.Num(); ++i)
    {
        if (Pool[i].bNeedsClear)
        {
            ClearSlot(i);
            PoolManager->ClearSlotFlag(i);
        }
    }
}

void URTDrawManager::ClearSlot(int32 SlotIndex) const
{
    const TArray<FRTPoolEntry>& Pool = PoolManager->GetPool();
    if (!Pool.IsValidIndex(SlotIndex) || !Pool[SlotIndex].InteractionRT) { return; }
    UKismetRenderingLibrary::ClearRenderTarget2D(
        GetWorld(), Pool[SlotIndex].InteractionRT, GInteractionNeutral);
}