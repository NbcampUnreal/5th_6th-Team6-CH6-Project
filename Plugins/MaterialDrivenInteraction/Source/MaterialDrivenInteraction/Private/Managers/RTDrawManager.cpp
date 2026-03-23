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

static const FLinearColor GContinuousNeutral(0.5f, 0.5f, 0.f, 0.f);

// for the clipping
enum class EBrushTileType : uint8
{
	Original = 0,
	Wrapped  = 1
};

bool URTDrawManager::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	return World && (World->IsGameWorld() || World->IsPlayInEditor());
}

void URTDrawManager::Initialize(FSubsystemCollectionBase& Collection)
{
	// Ensure URTPoolManager is initialized before this subsystem
	Collection.InitializeDependency<URTPoolManager>();// make this subsystem to be initialized first

	Super::Initialize(Collection);

	PoolManager = GetWorld()->GetSubsystem<URTPoolManager>();

	if (!PoolManager)
	{
		UE_LOG(RTFoliageInvoker, Error,
			TEXT("URTDrawManager::Initialize >> URTPoolManager not found"));
		return;
	}

	UE_LOG(RTFoliageInvoker, Log, TEXT("URTDrawManager::Initialize >> Ready"));
}

void URTDrawManager::Deinitialize()
{
	Super::Deinitialize();
}

void URTDrawManager::Update(float DeltaTime)
{
	if (!PoolManager)
	{
		UE_LOG(RTFoliageInvoker, Warning,
			TEXT("URTDrawManager::Update >> PoolManager is null — skipping"));
		return;
	}

	if (!GetWorld())
	{
		UE_LOG(RTFoliageInvoker, Warning,
			TEXT("URTDrawManager::Update >> World is null — skipping"));
		return;
	}

	const TArray<TTuple<UFoliageRTInvokerComponent*, int32>> AssignedInvokers =
		PoolManager->EvaluateAndAssignSlots();

	ProcessPendingImpulseClears();
	ClearContinuousRTs();
	DrawAllInvokers(AssignedInvokers, DeltaTime);
	PoolManager->ReclaimExpiredSlots();

	UE_LOG(RTFoliageInvoker, Verbose,
		TEXT("URTDrawManager::Update >> Drew %d invokers"), AssignedInvokers.Num());
}

void URTDrawManager::DrawAllInvokers(
	const TArray<TTuple<UFoliageRTInvokerComponent*, int32>>& AssignedInvokers,  float DeltaTime) const
{
	if (AssignedInvokers.Num() == 0) { return; }

	for (int32 SlotIdx = 0; SlotIdx < PoolManager->PoolSize; ++SlotIdx)
	{
		UTextureRenderTarget2D* ImpulseRT    = nullptr;
		UTextureRenderTarget2D* ContinuousRT = nullptr;
		FVector2D               DummyOrigin;

		if (!PoolManager->GetSlotData(SlotIdx, ImpulseRT, ContinuousRT, DummyOrigin))
		{
			continue;
		}

		if (ContinuousRT)
		{
			DrawContinuousForSlot(ContinuousRT, SlotIdx, AssignedInvokers);
		}
		if (ImpulseRT)
		{
			DrawImpulseForSlot(ImpulseRT, SlotIdx, AssignedInvokers);

			/*// the new method with decay progression
			DecayImpulseForSlot(ImpulseRT, DeltaTime);
			StampImpulseForSlot(ImpulseRT, SlotIdx, AssignedInvokers);*/
		}
	}
}

void URTDrawManager::DrawContinuousForSlot(UTextureRenderTarget2D* RT, int32 SlotIdx,
	const TArray<TTuple<UFoliageRTInvokerComponent*, int32>>& AssignedInvokers) const
{
	UCanvas* Canvas = nullptr;
	FVector2D CanvasSize;
	FDrawToRenderTargetContext Context;

	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(
		GetWorld(), RT, Canvas, CanvasSize, Context);

	if (Canvas)
	{
		for (const TTuple<UFoliageRTInvokerComponent*, int32>& Pair : AssignedInvokers)
		{
			if (Pair.Get<1>() != SlotIdx) { continue; }

			UFoliageRTInvokerComponent* Invoker = Pair.Get<0>();
			if (!Invoker) { continue; }

			FVector2D CellOriginWS;
			UTextureRenderTarget2D* Dummy1 = nullptr;
			UTextureRenderTarget2D* Dummy2 = nullptr;
			PoolManager->GetSlotData(SlotIdx, Dummy1, Dummy2, CellOriginWS);

			const FRTInvokerFrameData Data =
				Invoker->GetFrameData(SlotIdx, CellOriginWS, PoolManager->CellSize);

			if (!Data.MID_Continuous) { continue; }

			DrawTiledBrush(Canvas, CanvasSize, Data.MID_Continuous,
				Data.CellUV, Data.UVExtent, SE_BLEND_MAX );// height lerp , no drawing over other's normal

		}

		UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);
	}
}

void URTDrawManager::DrawImpulseForSlot(UTextureRenderTarget2D* RT, int32 SlotIdx,
	const TArray<TTuple<UFoliageRTInvokerComponent*, int32>>& AssignedInvokers) const
{
	UCanvas* Canvas = nullptr;
	FVector2D CanvasSize;
	FDrawToRenderTargetContext Context;

	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(
		GetWorld(), RT, Canvas, CanvasSize, Context);

	if (Canvas)
	{
		for (const TTuple<UFoliageRTInvokerComponent*, int32>& Pair : AssignedInvokers)
		{
			if (Pair.Get<1>() != SlotIdx) { continue; }

			UFoliageRTInvokerComponent* Invoker = Pair.Get<0>();
			if (!Invoker) { continue; }

			FVector2D CellOriginWS;
			UTextureRenderTarget2D* Dummy1 = nullptr;
			UTextureRenderTarget2D* Dummy2 = nullptr;
			PoolManager->GetSlotData(SlotIdx, Dummy1, Dummy2, CellOriginWS);

			const FRTInvokerFrameData Data =
				Invoker->GetFrameData(SlotIdx, CellOriginWS, PoolManager->CellSize);

			// DEBUG — remove bShouldStamp gate to verify RT is writing at all
			if (!Data.MID_Impulse) { continue; }

			UE_LOG(RTFoliageInvoker, Log,
				TEXT("URTDrawManager::DrawImpulseForSlot >> Drawing Slot %d UV(%.3f,%.3f) Stamp=%s"),
				SlotIdx, Data.CellUV.X, Data.CellUV.Y,
				Data.bShouldStamp ? TEXT("yes") : TEXT("no"));

			DrawTiledBrush(Canvas, CanvasSize, Data.MID_Impulse,
				Data.CellUV, Data.UVExtent, SE_BLEND_Translucent);
		}

		UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);
	}
}

void URTDrawManager::DrawTiledBrush(
    UCanvas* Canvas,
    FVector2D CanvasSize,
    UMaterialInstanceDynamic* MID,
    FVector2D CentreUV,
    FVector2D UVExtent,
    ESimpleElementBlendMode BlendMode) const
{
    if (!MID || !Canvas) { return; }

    struct FTile
    {
        FVector2D Offset;
        EBrushTileType Type;
    };

    TArray<FTile, TInlineAllocator<9>> Tiles;

    const bool bWrapLeft   = CentreUV.X - UVExtent.X < 0.f;
    const bool bWrapRight  = CentreUV.X + UVExtent.X > 1.f;
    const bool bWrapBottom = CentreUV.Y - UVExtent.Y < 0.f;
    const bool bWrapTop    = CentreUV.Y + UVExtent.Y > 1.f;

    // Original
    Tiles.Add({ FVector2D(0.f, 0.f), EBrushTileType::Original });

    // Wrapped
    if (bWrapLeft)   Tiles.Add({ FVector2D( 1.f,  0.f), EBrushTileType::Wrapped });
    if (bWrapRight)  Tiles.Add({ FVector2D(-1.f,  0.f), EBrushTileType::Wrapped });
    if (bWrapBottom) Tiles.Add({ FVector2D( 0.f,  1.f), EBrushTileType::Wrapped });
    if (bWrapTop)    Tiles.Add({ FVector2D( 0.f, -1.f), EBrushTileType::Wrapped });

    if (bWrapLeft  && bWrapBottom) Tiles.Add({ FVector2D( 1.f,  1.f), EBrushTileType::Wrapped });
    if (bWrapRight && bWrapBottom) Tiles.Add({ FVector2D(-1.f,  1.f), EBrushTileType::Wrapped });
    if (bWrapLeft  && bWrapTop)    Tiles.Add({ FVector2D( 1.f, -1.f), EBrushTileType::Wrapped });
    if (bWrapRight && bWrapTop)    Tiles.Add({ FVector2D(-1.f, -1.f), EBrushTileType::Wrapped });

    // 🔥 Pixel-safe inset (screen space)
    const FVector2D PixelInset(0.5f, 0.5f);
    const FVector2D DrawPos  = PixelInset;
    const FVector2D DrawSize = CanvasSize - PixelInset * 2.0f;

    // 🔥 UV-safe inset (this is the missing piece)
    const FVector2D UVInset(
        0.5f / CanvasSize.X,
        0.5f / CanvasSize.Y);

    const FVector2D UV0 = FVector2D(0.f, 0.f) + UVInset;
    const FVector2D UV1 = FVector2D(1.f, 1.f) - UVInset;

    for (const FTile& Tile : Tiles)
    {
        FVector2D WrappedCenter = CentreUV + Tile.Offset;

        MID->SetVectorParameterValue(
            FName("BrushCentreUV"),
            FLinearColor(WrappedCenter.X, WrappedCenter.Y, 0.f, 0.f));

        MID->SetVectorParameterValue(
            FName("BrushExtent"),
            FLinearColor(UVExtent.X, UVExtent.Y, 0.f, 0.f));

        MID->SetScalarParameterValue(
            FName("TileType"),
            (float)Tile.Type);

        FCanvasTileItem TileItem(
            DrawPos,
            MID->GetRenderProxy(),
            DrawSize);
    	
        /*TileItem.UV0 = UV0;
        TileItem.UV1 = UV1;*/

    	constexpr float RTMargin = 0.01f;
    	TileItem.UV0 = FVector2D(RTMargin, RTMargin);
        TileItem.UV1 = FVector2D(1.f-RTMargin, 1.f-RTMargin);
        
        TileItem.BlendMode = BlendMode;

        Canvas->DrawItem(TileItem);
    }
}

void URTDrawManager::ProcessPendingImpulseClears() const
{
	const TArray<FRTPoolEntry>& Pool = PoolManager->GetPool();
	for (int32 i = 0; i < Pool.Num(); ++i)
	{
		if (Pool[i].bImpulseNeedsClear)
		{
			ClearImpulseSlot(i);
			PoolManager->ClearImpulseFlag(i);
		}
	}
}

void URTDrawManager::ClearContinuousRTs() const
{
	const TArray<FRTPoolEntry>& Pool = PoolManager->GetPool();
	for (int32 i = 0; i < Pool.Num(); ++i)
	{
		if (Pool[i].IsOccupied() && Pool[i].ContinuousRT)
		{
			ClearContinuousSlot(i);
		}
	}
}

void URTDrawManager::ClearImpulseSlot(int32 SlotIndex) const
{
	const TArray<FRTPoolEntry>& Pool = PoolManager->GetPool();
	if (!Pool.IsValidIndex(SlotIndex) || !Pool[SlotIndex].ImpulseRT) { return; }

	UKismetRenderingLibrary::ClearRenderTarget2D(
		GetWorld(), Pool[SlotIndex].ImpulseRT, FLinearColor::Black);

	UE_LOG(RTFoliageInvoker, Verbose,
		TEXT("URTDrawManager::ClearImpulseSlot >> Slot %d cleared"), SlotIndex);
}

void URTDrawManager::ClearContinuousSlot(int32 SlotIndex) const
{
	const TArray<FRTPoolEntry>& Pool = PoolManager->GetPool();
	if (!Pool.IsValidIndex(SlotIndex) || !Pool[SlotIndex].ContinuousRT) { return; }

	UKismetRenderingLibrary::ClearRenderTarget2D(
		GetWorld(), Pool[SlotIndex].ContinuousRT, GContinuousNeutral);
}


//Temp testing
void URTDrawManager::DecayImpulseForSlot(UTextureRenderTarget2D* RT, float DeltaTime) const
{
	if (!RT) { return; }

	UCanvas* Canvas = nullptr;
	FVector2D CanvasSize;
	FDrawToRenderTargetContext Context;

	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(
		GetWorld(), RT, Canvas, CanvasSize, Context);

	if (Canvas)
	{
		// Add DecayDelta to all pixels — A climbs from 0 toward 1 over time
		const float DecayRate  = 1.f / FMath::Max(PoolManager->DecayDuration, 0.001f);
		const float DecayDelta = DeltaTime * DecayRate;

		FCanvasTileItem TileItem(
			FVector2D(0.f, 0.f),
			GWhiteTexture,
			CanvasSize,
			FVector2D(0.f, 0.f),
			FVector2D(1.f, 1.f),
			FLinearColor(DecayDelta, DecayDelta, DecayDelta, DecayDelta));
		// use the decay value for the overlay

		TileItem.BlendMode = SE_BLEND_Additive;
		Canvas->DrawItem(TileItem);

		UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);
	}
}

void URTDrawManager::StampImpulseForSlot(UTextureRenderTarget2D* RT, int32 SlotIdx,
	const TArray<TTuple<UFoliageRTInvokerComponent*, int32>>& AssignedInvokers) const
{
	UCanvas* Canvas = nullptr;
	FVector2D CanvasSize;
	FDrawToRenderTargetContext Context;

	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(
		GetWorld(), RT, Canvas, CanvasSize, Context);

	if (Canvas)
	{
		for (const TTuple<UFoliageRTInvokerComponent*, int32>& Pair : AssignedInvokers)
		{
			if (Pair.Get<1>() != SlotIdx) { continue; }

			UFoliageRTInvokerComponent* Invoker = Pair.Get<0>();
			if (!Invoker) { continue; }

			FVector2D CellOriginWS;
			UTextureRenderTarget2D* Dummy1 = nullptr;
			UTextureRenderTarget2D* Dummy2 = nullptr;
			PoolManager->GetSlotData(SlotIdx, Dummy1, Dummy2, CellOriginWS);

			const FRTInvokerFrameData Data =
				Invoker->GetFrameData(SlotIdx, CellOriginWS, PoolManager->CellSize);

			if (!Data.bShouldStamp || !Data.MID_Impulse) { continue; }

			// Stamp resets progression to 0 at brush position — Opaque black overdraw
			DrawTiledBrush(Canvas, CanvasSize, Data.MID_Impulse,
				Data.CellUV, Data.UVExtent, SE_BLEND_Opaque);
		}

		UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);
	}
}