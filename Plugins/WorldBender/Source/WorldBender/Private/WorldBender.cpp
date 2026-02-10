// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldBender.h"
#include "Interfaces/IPluginManager.h"//for IPluginManager::Get()
#include "Misc/Paths.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FWorldBenderModule"

void FWorldBenderModule::StartupModule()
{
	AddShaderSourceDirectoryMapping(
    	TEXT("/Plugin/WorldBender"),
    		FPaths::Combine(
    			IPluginManager::Get().FindPlugin(TEXT("WorldBender"))->GetBaseDir(),
    			TEXT("Shaders")
    		)
    	);
}

void FWorldBenderModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FWorldBenderModule, WorldBender)