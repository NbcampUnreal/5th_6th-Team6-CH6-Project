#include "DoubleRT/DoubleBufferRT.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Engine/World.h"

//Log
DEFINE_LOG_CATEGORY(DoubleRTBuffer);

void UDoubleBufferRT::Initialize(
    UCanvasRenderTarget2D*        InFrontRT,
    UCanvasRenderTarget2D*        InBackRT,
    UMaterialInstanceDynamic*     InMID,
    UMaterialParameterCollection* InMPC,
    FVector                       InInitialWorldLocation)
{
    if (!InFrontRT || !InBackRT || !InMID)
    {
        UE_LOG(DoubleRTBuffer, Warning,
            TEXT("UDoubleBufferRT::Initialize >> Missing RT or MID — aborted"));
        return;
    }

    if (InFrontRT == InBackRT)
    {
        UE_LOG(DoubleRTBuffer, Warning,
            TEXT("UDoubleBufferRT::Initialize >> Front and Back RT must be different assets"));
        return;
    }

    RT_A      = InFrontRT;
    RT_B      = InBackRT;
    TargetMID = InMID;

    if (InMPC)
    {
        if (UWorld* World = GetOuter()->GetWorld())
        {
            MPCInstance = World->GetParameterCollectionInstance(InMPC);

            if (!MPCInstance)
            {
                UE_LOG(DoubleRTBuffer, Warning,
                    TEXT("UDoubleBufferRT::Initialize >> Failed to resolve MPC instance"));
            }
        }
    }

    bFrontIsA         = true;
    CurrentBlendAlpha = 1.f;
    TimeSinceLastDraw = 0.f;

    // Stable initial state — no pop on first frame
    TargetMID->SetTextureParameterValue(Param_CurrentRT,  RT_A);
    TargetMID->SetTextureParameterValue(Param_PreviousRT, RT_A);
    TargetMID->SetScalarParameterValue(Param_BlendAlpha,  1.f);

    // Both location params start at the same position — zero offset in material
    if (MPCInstance)
    {
        const FLinearColor InitLoc(
            InInitialWorldLocation.X,
            InInitialWorldLocation.Y,
            InInitialWorldLocation.Z);

        MPCInstance->SetVectorParameterValue(MPC_OwnerLocation,    InitLoc);
        MPCInstance->SetVectorParameterValue(MPC_LastDrawLocation, InitLoc);
    }

    bInitialized = true;

    UE_LOG(DoubleRTBuffer, Log,
        TEXT("UDoubleBufferRT::Initialize >> Ready | Interval: %.2fs | BlendSpeed: %.1f | MPC: %s"),
        DrawInterval, BlendSpeed, MPCInstance ? TEXT("bound") : TEXT("none"));
}

// ── Per-frame ─────────────────────────────────────────────────────────────────

void UDoubleBufferRT::TickBuffer(float DeltaTime)
{
    if (!bInitialized) return;

    // ── Blend ─────────────────────────────────────────────────────────────

    CurrentBlendAlpha = FMath::FInterpTo(CurrentBlendAlpha, 1.f, DeltaTime, BlendSpeed);
    PushBlendAlpha(CurrentBlendAlpha);

    OnFrameUpdate.Broadcast();

    // ── Swap interval ─────────────────────────────────────────────────────

    TimeSinceLastDraw += DeltaTime;
    if (TimeSinceLastDraw < DrawInterval) return;

    TimeSinceLastDraw = 0.f;

    // Snapshot front as previous before overwriting
    TargetMID->SetTextureParameterValue(Param_PreviousRT, GetFrontRT());
    TargetMID->SetTextureParameterValue(Param_CurrentRT,  GetBackRT());

    SwapBuffers();

    // Blend resets — consumer draws into fresh back buffer
    CurrentBlendAlpha = 0.f;
    PushBlendAlpha(0.f);

    // Lock MPC last draw location to current owner location —
    // material UV offset resets to zero until pawn moves again
    SnapshotDrawLocation();

    OnDrawToBackBuffer.Broadcast(GetBackRT());
}

void UDoubleBufferRT::UpdateLocationMPC(FVector CurrentWorldLocation, float WorldHalfExtent) const
{
    if (!MPCInstance) return;

    MPCInstance->SetVectorParameterValue(MPC_OwnerLocation,
        FLinearColor(CurrentWorldLocation.X, CurrentWorldLocation.Y, CurrentWorldLocation.Z));

    MPCInstance->SetScalarParameterValue(MPC_WorldHalfExtent, WorldHalfExtent);
}

// ── Accessors ─────────────────────────────────────────────────────────────────

UCanvasRenderTarget2D* UDoubleBufferRT::GetFrontRT() const
{
    return bFrontIsA ? RT_A : RT_B;
}

UCanvasRenderTarget2D* UDoubleBufferRT::GetBackRT() const
{
    return bFrontIsA ? RT_B : RT_A;
}

// ── Private ───────────────────────────────────────────────────────────────────

void UDoubleBufferRT::SwapBuffers()
{
    bFrontIsA = !bFrontIsA;
}

void UDoubleBufferRT::PushBlendAlpha(float BlendAlpha) const
{
    if (!TargetMID) return;
    TargetMID->SetScalarParameterValue(Param_BlendAlpha, BlendAlpha);
}

void UDoubleBufferRT::SnapshotDrawLocation() const
{
    if (!MPCInstance) return;

    // Read current owner location back from MPC and lock it as the last draw position
    FLinearColor CurrentOwnerLoc;
    MPCInstance->GetVectorParameterValue(MPC_OwnerLocation, CurrentOwnerLoc);
    MPCInstance->SetVectorParameterValue(MPC_LastDrawLocation, CurrentOwnerLoc);
}