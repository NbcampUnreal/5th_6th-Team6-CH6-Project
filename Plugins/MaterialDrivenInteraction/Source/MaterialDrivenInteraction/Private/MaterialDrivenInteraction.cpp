// Copyright Epic Games, Inc. All Rights Reserved.

#include "MaterialDrivenInteraction.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"


#define LOCTEXT_NAMESPACE "FMaterialDrivenInteractionModule"

void FMaterialDrivenInteractionModule::StartupModule()
{
	//   #include "/FoliageRT/FoliageRT.ush"
	const TSharedPtr<IPlugin> Plugin =
		IPluginManager::Get().FindPlugin(TEXT("MaterialDrivenInteraction"));

	if (ensureMsgf(Plugin.IsValid(),
		TEXT("[FoliageRT] Could not find plugin 'MaterialDrivenInteraction' — "
			 "shader include path not registered. Custom nodes will fail to compile.")))
	{
		const FString ShaderDir =
			FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));

		AddShaderSourceDirectoryMapping(TEXT("/FoliageRT"), ShaderDir);
	}
}

void FMaterialDrivenInteractionModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMaterialDrivenInteractionModule, MaterialDrivenInteraction)
