#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "DoubleBufferRT.generated.h"

// FD
class UMaterialInstanceDynamic;
class UMaterialParameterCollection;
class UMaterialParameterCollectionInstance;

// Delegate
DOUBLERTBUFFERDRAWER_API DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDrawToBackBuffer, UCanvasRenderTarget2D*, BackRT);
DOUBLERTBUFFERDRAWER_API DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFrameUpdate);

// Log

DOUBLERTBUFFERDRAWER_API DECLARE_LOG_CATEGORY_EXTERN(DoubleRTBuffer, Log, All);

UCLASS(BlueprintType)
class DOUBLERTBUFFERDRAWER_API UDoubleBufferRT : public UObject
{
    GENERATED_BODY()

public:

    // ── Setup ─────────────────────────────────────────────────────────────

    /** Call once after all references are ready.
     *  MID must expose: RT_Current, RT_Previous, BlendAlpha.
     *  MPC must expose: OwnerWorldLocation, LastDrawWorldLocation, RTWorldHalfExtent. */
    UFUNCTION(BlueprintCallable, Category="DoubleBuffer")
    void Initialize(
        UCanvasRenderTarget2D*        InFrontRT,
        UCanvasRenderTarget2D*        InBackRT,
        UMaterialInstanceDynamic*     InMID,
        UMaterialParameterCollection* InMPC,
        FVector                       InInitialWorldLocation);

    // ── Per-frame ─────────────────────────────────────────────────────────

    /** Drives blend alpha and swap interval — call every frame.
     *  Does NOT touch location — call UpdateLocationMPC separately. */
    UFUNCTION(BlueprintCallable, Category="DoubleBuffer")
    void TickBuffer(float DeltaTime);

    /** Updates MPC owner location and half extent — call every frame before TickBuffer.
     *  One call covers all drawers sharing the same MPC. */
    UFUNCTION(BlueprintCallable, Category="DoubleBuffer")
    void UpdateLocationMPC(FVector CurrentWorldLocation, float WorldHalfExtent) const;

    // ── Delegates ─────────────────────────────────────────────────────────

    /** Fired once per draw interval — bind your stamp/paint logic here */
    UPROPERTY(BlueprintAssignable, Category="DoubleBuffer")
    FOnDrawToBackBuffer OnDrawToBackBuffer;

    /** Fired every TickBuffer call — bind per-frame updates here */
    UPROPERTY(BlueprintAssignable, Category="DoubleBuffer")
    FOnFrameUpdate OnFrameUpdate;

    // ── Config ────────────────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DoubleBuffer")
    float DrawInterval = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DoubleBuffer")
    float BlendSpeed = 8.f;

    // ── MID param names ───────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DoubleBuffer|MID Params")
    FName Param_CurrentRT = TEXT("RT_Current");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DoubleBuffer|MID Params")
    FName Param_PreviousRT = TEXT("RT_Previous");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DoubleBuffer|MID Params")
    FName Param_BlendAlpha = TEXT("BlendAlpha");

    // ── MPC param names ───────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DoubleBuffer|MPC Params")
    FName MPC_OwnerLocation = TEXT("OwnerWorldLocation");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DoubleBuffer|MPC Params")
    FName MPC_LastDrawLocation = TEXT("LastDrawWorldLocation");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DoubleBuffer|MPC Params")
    FName MPC_WorldHalfExtent = TEXT("RTWorldHalfExtent");

    // ── Accessors ─────────────────────────────────────────────────────────

    UFUNCTION(BlueprintPure, Category="DoubleBuffer")
    UCanvasRenderTarget2D* GetFrontRT() const;

    UFUNCTION(BlueprintPure, Category="DoubleBuffer")
    UCanvasRenderTarget2D* GetBackRT() const;

    UFUNCTION(BlueprintPure, Category="DoubleBuffer")
    bool IsInitialized() const { return bInitialized; }

private:

    void SwapBuffers();
    void PushBlendAlpha(float BlendAlpha) const;
    void SnapshotDrawLocation() const;

    UPROPERTY()
    TObjectPtr<UCanvasRenderTarget2D> RT_A = nullptr;

    UPROPERTY()
    TObjectPtr<UCanvasRenderTarget2D> RT_B = nullptr;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> TargetMID = nullptr;

    UPROPERTY()
    TObjectPtr<UMaterialParameterCollectionInstance> MPCInstance = nullptr;

    bool  bFrontIsA         = true;
    bool  bInitialized      = false;
    float TimeSinceLastDraw = 0.f;
    float CurrentBlendAlpha = 1.f;
};