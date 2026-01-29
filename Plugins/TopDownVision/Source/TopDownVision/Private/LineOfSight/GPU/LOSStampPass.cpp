// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/GPU/LOSStampPass.h"

#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"

IMPLEMENT_GLOBAL_SHADER(
	FLOSStampCS,
	"/Plugin/TopDownVision/RayMarch/LOSStamp.usf", // not project! this is plugin
	"MainCS",
	SF_Compute);

FRDGTextureRef CreateLOSTexture(FRDGBuilder& GraphBuilder, FIntPoint Size)
{
	FRDGTextureDesc Desc = FRDGTextureDesc::Create2D(
		Size,
		PF_R32_FLOAT,
		FClearValueBinding::Black,
		TexCreate_ShaderResource | TexCreate_UAV
	);

	return GraphBuilder.CreateTexture(Desc, TEXT("LOS_Texture"));
}

void AddLOSStampPass(
	FRDGBuilder& GraphBuilder,
	FRDGTextureRef LOSTexture,
	const TArray<FLOSStampData>& Stamps,
	uint32 ViewChannelMask,
	bool bClearBeforeStamp
)
{
	if (!LOSTexture||Stamps.Num() == 0)
	{
		return;
	}

	// clear if checked
	if (bClearBeforeStamp)
	{
		AddClearUAVPass(
			GraphBuilder,
			GraphBuilder.CreateUAV(LOSTexture),
			0.0f
		);
	}

	// Create structured buffer
	FRDGBufferRef StampBuffer = GraphBuilder.CreateBuffer(
		FRDGBufferDesc::CreateStructuredDesc(
			sizeof(FLOSStampData),
			Stamps.Num()
		),
		TEXT("LOS.StampBuffer")
	);

	// Upload data
	GraphBuilder.QueueBufferUpload(
		StampBuffer,
		Stamps.GetData(),
		sizeof(FLOSStampData) * Stamps.Num()
	);

	TShaderMapRef<FLOSStampCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	
	FLOSStampCS::FParameters* Params = GraphBuilder.AllocParameters<FLOSStampCS::FParameters>();
	Params->LOSStamps = GraphBuilder.CreateSRV(StampBuffer);
	Params->NumStamps = Stamps.Num();
	Params->ViewChannelMask = ViewChannelMask;
	Params->LOSOutput = GraphBuilder.CreateUAV(LOSTexture);

	FIntPoint Size = LOSTexture->Desc.Extent;
	FIntVector GroupCount(
		FMath::DivideAndRoundUp(Size.X, 8),
		FMath::DivideAndRoundUp(Size.Y, 8),
		1
	);

	GraphBuilder.AddPass(
		RDG_EVENT_NAME("LOS_Stamp_Batched"),
		Params,
		ERDGPassFlags::Compute,
		[Params, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList)
		{
			FComputeShaderUtils::Dispatch(
				RHICmdList,
				ComputeShader,
				*Params,
				GroupCount
			);
		}
	);
}