// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DoubleRTBufferDrawer : ModuleRules
{
	public DoubleRTBufferDrawer(ReadOnlyTargetRules Target) : base(Target)
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
			
		
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",

			// UCanvasRenderTarget2D, UCanvas, FCanvasTileItem
			"RenderCore",
			"Renderer",

			// UMaterialInstanceDynamic
			"MaterialShaderQualitySettings",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			// FRDGBuilder, render graph utils — needed if consumers use GPU path
			"RHI",
			"RenderCore",
			"Renderer",

			// UKismetRenderingLibrary
			"Engine",
		});
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
