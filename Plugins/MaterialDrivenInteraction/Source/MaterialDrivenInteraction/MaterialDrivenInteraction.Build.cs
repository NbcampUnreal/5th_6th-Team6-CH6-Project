// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MaterialDrivenInteraction : ModuleRules
{
	public MaterialDrivenInteraction(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"MaterialShaderQualitySettings",
				"Projects",       // IPluginManager
				"DeveloperSettings",// for editor setting
				
				"CustomBlendModeForRTDraw", 
				//!!!!!! For Custom Blend Mode for encoding Locomotion data to Rt with no loss or modification by unreal
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"RenderCore",   // ETextureRenderTargetFormat
				"RHI",          // FRHICommandListImmediate 
				//"Settings",     // required for ISettingsModule // just use developer settings class instead
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
