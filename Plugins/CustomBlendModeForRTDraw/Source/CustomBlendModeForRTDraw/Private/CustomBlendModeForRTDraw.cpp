// Copyright Epic Games, Inc. All Rights Reserved.

#include "CustomBlendModeForRTDraw.h"

//shader module
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FCustomBlendModeForRTDrawModule"

void FCustomBlendModeForRTDrawModule::StartupModule()
{
	// Register shader directory so .ush files are accessible
	FString PluginShaderDir = FPaths::Combine(
		IPluginManager::Get().FindPlugin(TEXT("CustomBlendModeForRTDraw"))->GetBaseDir(),
		TEXT("Shaders"));

	AddShaderSourceDirectoryMapping(
		TEXT("/CustomBlendModeForRTDraw"),
		PluginShaderDir);
}

void FCustomBlendModeForRTDrawModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCustomBlendModeForRTDrawModule, CustomBlendModeForRTDraw)