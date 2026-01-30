#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameters.h"
#include "RHI.h"

struct FTileGPUData
{
    FVector2f TileWorldMin;
    FVector2f TileWorldMax;
    uint32 TextureIndex;
    uint32 Padding[3];
    
    FTileGPUData()
        : TileWorldMin(FVector2f::ZeroVector)
        , TileWorldMax(FVector2f::ZeroVector)
        , TextureIndex(0)
    {
        Padding[0] = Padding[1] = Padding[2] = 0;
    }
};

class FTileMergeComputeShader : public FGlobalShader
{
    DECLARE_SHADER_TYPE(FTileMergeComputeShader, Global);

public:
    FTileMergeComputeShader() {}
    
    FTileMergeComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FGlobalShader(Initializer)
    {
        OutputTexture.Bind(Initializer.ParameterMap, TEXT("OutputTexture"));
        TileInfoBuffer.Bind(Initializer.ParameterMap, TEXT("TileInfoBuffer"));
        TileSampler.Bind(Initializer.ParameterMap, TEXT("TileSampler"));
        LocalWorldMin.Bind(Initializer.ParameterMap, TEXT("LocalWorldMin"));
        LocalWorldMax.Bind(Initializer.ParameterMap, TEXT("LocalWorldMax"));
        TileCount.Bind(Initializer.ParameterMap, TEXT("TileCount"));
        OutputResolution.Bind(Initializer.ParameterMap, TEXT("OutputResolution"));
        
        // Bind texture array manually
        TileTexturesParam.Bind(Initializer.ParameterMap, TEXT("TileTextures"));
    }

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }

    static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, 
        FShaderCompilerEnvironment& OutEnvironment)
    {
        FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
        OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE"), 8);
        OutEnvironment.SetDefine(TEXT("MAX_TILES"), 32);
    }

    void SetParameters(
        FRHIBatchedShaderParameters& BatchedParameters,
        FRHIUnorderedAccessView* OutputUAV,
        FRHIShaderResourceView* TileInfoSRV,
        const TArray<FRHITexture*>& Textures,
        FRHISamplerState* SamplerState,
        const FVector2f& InLocalWorldMin,
        const FVector2f& InLocalWorldMax,
        uint32 InTileCount,
        uint32 InResolution)
    {
        SetUAVParameter(BatchedParameters, OutputTexture, OutputUAV);
        SetSRVParameter(BatchedParameters, TileInfoBuffer, TileInfoSRV);
        SetSamplerParameter(BatchedParameters, TileSampler, SamplerState);
        
        SetShaderValue(BatchedParameters, LocalWorldMin, InLocalWorldMin);
        SetShaderValue(BatchedParameters, LocalWorldMax, InLocalWorldMax);
        SetShaderValue(BatchedParameters, TileCount, InTileCount);
        SetShaderValue(BatchedParameters, OutputResolution, InResolution);
        
        // Set texture array
        if (TileTexturesParam.IsBound())
        {
            for (int32 i = 0; i < FMath::Min(Textures.Num(), 32); ++i)
            {
                if (Textures[i])
                {
                    SetTextureParameter(BatchedParameters, TileTexturesParam, Textures[i]);
                }
            }
        }
    }

private:
    LAYOUT_FIELD(FShaderResourceParameter, OutputTexture);
    LAYOUT_FIELD(FShaderResourceParameter, TileInfoBuffer);
    LAYOUT_FIELD(FShaderResourceParameter, TileSampler);
    LAYOUT_FIELD(FShaderParameter, LocalWorldMin);
    LAYOUT_FIELD(FShaderParameter, LocalWorldMax);
    LAYOUT_FIELD(FShaderParameter, TileCount);
    LAYOUT_FIELD(FShaderParameter, OutputResolution);
    LAYOUT_FIELD(FShaderResourceParameter, TileTexturesParam); // Array binding
};

namespace TileMergeCS
{
    void Execute_RenderThread(
        FRHICommandListImmediate& RHICmdList,
        const TArray<FTileGPUData>& TileData,
        const TArray<FTextureRHIRef>& TileTextures,
        FRHITexture* OutputTexture,
        const FVector2f& LocalWorldMin,
        const FVector2f& LocalWorldMax,
        uint32 OutputResolution);
}