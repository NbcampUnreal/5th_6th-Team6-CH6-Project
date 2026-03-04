// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LocalTextureSampler.generated.h"

/*
 * This is Texture renderer, which samples the world texture into local texture
 *
 * it is for replacing a heavy 2dScene capture component. no more capturing per tick
 *
 * it gathers the overlapping pre-baked height texture and merge it into one local texture
 *
 * No tick-based capture
 * No collision or overlap queries
 * Pure AABB math + GPU projection
 * GPU-only merge using material passes
 *
 * fuck yeah
 */

class UTextureRenderTarget2D;
class UWorldObstacleSubsystem;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API ULocalTextureSampler : public UActorComponent
{
    GENERATED_BODY()

public:
    ULocalTextureSampler();

protected:
    virtual void BeginPlay() override;
    virtual void OnComponentCreated() override;

public:
    /** Force update of the local texture (teleport, vision change, etc.) */
    UFUNCTION(BlueprintCallable, Category="LocalSampler")
    void UpdateLocalTexture();

    /** Change sampling radius (will trigger re-sample) */
    UFUNCTION(BlueprintCallable, Category="LocalSampler")
    void SetWorldSampleRadius(float NewRadius);

    UFUNCTION(BlueprintCallable, Category="LocalSampler")
    void SetLocalRenderTarget(UTextureRenderTarget2D* InRT);

    UFUNCTION(BlueprintCallable, Category="LocalSampler")
    UTextureRenderTarget2D* GetLocalRenderTarget() const { return LocalMaskRT; }

    void SetLocationRoot(USceneComponent* NewRoot);

    UFUNCTION(BlueprintCallable, Category="LocalSampler")
    void PrepareSetups();

    UFUNCTION(BlueprintCallable, Category="LocalSampler")
    bool ShouldRunClientLogic() const;

    /** Set the world explicitly — required when nested inside another component */
    UFUNCTION(BlueprintCallable, Category="LocalSampler")
    void SetCachedWorld(UWorld* InWorld);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LocalSampler|Render")
    bool bDrawDebugRT = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LocalSampler|Render")
    TObjectPtr<UTextureRenderTarget2D> DebugRT;

    /** Local merged obstacle/height mask */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LocalSampler|Render")
    TObjectPtr<UTextureRenderTarget2D> LocalMaskRT;

    /** World-space radius of the local sampling area */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LocalSampler|Settings")
    float WorldSampleRadius = 512.f;

    /** Resolution of the local render target */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LocalSampler|Settings")
    int32 LocalResolution = 256;

    UPROPERTY(Transient)
    FVector LastSampleCenter = FVector::ZeroVector;

    UPROPERTY(Transient)
    FBox2D LocalWorldBounds;

    UPROPERTY(Transient)
    TArray<int32> ActiveTileIndices;

private:
    void RebuildLocalBounds(const FVector& WorldCenter);
    void UpdateOverlappingTiles();
    void DrawTilesIntoLocalRT();

    /** Returns CachedWorld if set, falls back to GetWorld(). Logs warning if both null. */
    UWorld* ResolveWorld() const;

    UPROPERTY(Transient)
    TObjectPtr<UWorldObstacleSubsystem> ObstacleSubsystem;

    UPROPERTY(Transient)
    TWeakObjectPtr<USceneComponent> SourceRoot;

    UPROPERTY(Transient)
    UWorld* CachedWorld = nullptr;
};
