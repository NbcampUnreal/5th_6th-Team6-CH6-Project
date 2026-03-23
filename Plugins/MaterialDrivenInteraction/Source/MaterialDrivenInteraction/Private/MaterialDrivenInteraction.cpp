// Copyright Epic Games, Inc. All Rights Reserved.

#include "MaterialDrivenInteraction.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#include "Data/RTPoolTypes.h"

#if WITH_EDITOR
#include "ISettingsModule.h" // for rt settings
#endif

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
		AddShaderSourceDirectoryMapping(TEXT("/TargetBrush"), ShaderDir);
	}


#if WITH_EDITOR
	// Register URTPoolSettings in Project Settings → Plugins → Foliage RT Pool
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings(
			"Project",
			"Plugins",
			"FoliageRTPool",
			LOCTEXT("FoliageRTPoolSettingsName", "Foliage RT Pool"),
			LOCTEXT("FoliageRTPoolSettingsDesc", "Configure render target pool size, cell size, decay duration, and RT asset assignments."),
			GetMutableDefault<URTPoolSettings>()
		);
	}
#endif
}

void FMaterialDrivenInteractionModule::ShutdownModule()
{
	
#if WITH_EDITOR
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings(
			"Project",
			"Plugins",
			"FoliageRTPool");
	}
#endif
	
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMaterialDrivenInteractionModule, MaterialDrivenInteraction)
