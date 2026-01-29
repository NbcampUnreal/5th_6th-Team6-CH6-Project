// Fill out your copyright notice in the Description page of Project Settings.


#pragma once

#include "RenderGraphResources.h"
#include "ShaderParameterStruct.h"
#include "GlobalShader.h"
#include "LineOfSight/GPU/LOSStampData.h"

class FLOSStampCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FLOSStampCS);
	SHADER_USE_PARAMETER_STRUCT(FLOSStampCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FLOSStampData>, LOSStamps)
		SHADER_PARAMETER(uint32, NumStamps)
		SHADER_PARAMETER(uint32, ViewChannelMask)// now a bit mask, not field
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, LOSOutput)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

FRDGTextureRef CreateLOSTexture(FRDGBuilder& GraphBuilder, FIntPoint Size);

void AddLOSStampPass(
	FRDGBuilder& GraphBuilder,
	FRDGTextureRef LOSTexture,
	const TArray<FLOSStampData>& Stamps,
	uint32 ViewChannelMask,
	bool bClearBeforeStamp
);