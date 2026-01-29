// Copyright Epic Games, Inc. All Rights Reserved.

#include "TopDownVision.h"
//Add Shader directory for material node
#include "ShaderCore.h"
#include "Interfaces/IPluginManager.h"
#include "LineOfSight/GPU/LOSStampPass.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "FTopDownVisionModule"

void FTopDownVisionModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	AddShaderSourceDirectoryMapping(
	TEXT("/Plugin/TopDownVision"),
		FPaths::Combine(
			IPluginManager::Get().FindPlugin(TEXT("TopDownVision"))->GetBaseDir(),
			TEXT("Shaders")
		)
	);
}

void FTopDownVisionModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTopDownVisionModule, TopDownVision)