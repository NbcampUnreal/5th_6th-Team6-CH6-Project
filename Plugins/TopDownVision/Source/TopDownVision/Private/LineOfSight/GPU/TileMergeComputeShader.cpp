

#include "LineOfSight/GPU/TileMergeComputeShader.h"

#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"

IMPLEMENT_SHADER_TYPE(, FTileMergeComputeShader, 
    TEXT("/Plugin/TopDownVision/Private/TileMergeComputeShader.usf"), 
    TEXT("MergeTilesCS"), 
    SF_Compute);

namespace TileMergeCS
{
    void Execute_RenderThread(
        FRHICommandListImmediate& RHICmdList,
        const TArray<FTileGPUData>& TileData,
        const TArray<FTextureRHIRef>& TileTextures,
        FRHITexture* OutputTexture,
        const FVector2f& LocalWorldMin,
        const FVector2f& LocalWorldMax,
        uint32 OutputResolution)
    {
        check(IsInRenderingThread());
        
        if (TileData.Num() == 0 || !OutputTexture)
            return;

        // Create structured buffer
        FRHIResourceCreateInfo CreateInfo(TEXT("TileInfoBuffer"));
        FBufferRHIRef TileInfoBuffer = RHICmdList.CreateStructuredBuffer(
            sizeof(FTileGPUData),
            TileData.Num() * sizeof(FTileGPUData),
            BUF_ShaderResource | BUF_Volatile,
            CreateInfo
        );

        // Upload data
        void* BufferData = RHICmdList.LockBuffer(
            TileInfoBuffer, 
            0, 
            TileData.Num() * sizeof(FTileGPUData), 
            RLM_WriteOnly
        );
        FMemory::Memcpy(BufferData, TileData.GetData(), TileData.Num() * sizeof(FTileGPUData));
        RHICmdList.UnlockBuffer(TileInfoBuffer);

        // Create views
        FShaderResourceViewRHIRef TileInfoSRV = RHICmdList.CreateShaderResourceView(TileInfoBuffer);
        FUnorderedAccessViewRHIRef OutputUAV = RHICmdList.CreateUnorderedAccessView(OutputTexture, 0);

        // Get shader
        TShaderMapRef<FTileMergeComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
        
        // Set compute shader
        RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

        // Prepare texture array
        TArray<FRHITexture*> TextureArray;
        TextureArray.Reserve(TileTextures.Num());
        for (const FTextureRHIRef& Tex : TileTextures)
        {
            TextureArray.Add(Tex.GetReference());
        }

        // Get batched parameters
        FRHIBatchedShaderParameters& BatchedParameters = RHICmdList.GetScratchShaderParameters();
        
        // Set parameters
        ComputeShader->SetParameters(
            BatchedParameters,
            OutputUAV,
            TileInfoSRV,
            TextureArray,
            TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp>::GetRHI(),
            LocalWorldMin,
            LocalWorldMax,
            TileData.Num(),
            OutputResolution
        );

        // Apply parameters
        RHICmdList.SetBatchedShaderParameters(ComputeShader.GetComputeShader(), BatchedParameters);

        // Dispatch
        const uint32 GroupCountX = FMath::DivideAndRoundUp(OutputResolution, 8u);
        const uint32 GroupCountY = FMath::DivideAndRoundUp(OutputResolution, 8u);
        RHICmdList.DispatchComputeShader(GroupCountX, GroupCountY, 1);

        // Unbind UAV
        FRHIBatchedShaderParameters& UnbindParams = RHICmdList.GetScratchShaderParameters();
        SetUAVParameter(UnbindParams, ComputeShader->OutputTexture, nullptr);
        RHICmdList.SetBatchedShaderParameters(ComputeShader.GetComputeShader(), UnbindParams);
    }
}