// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO; 

public class HexGridPlugin : ModuleRules
{
	public HexGridPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange( new string[] 
		{
			//PathFinding Paths
			/*"PathfindingLibrary/Public",
			"PathfindingLibrary/Public/CPU",*/
		});
				
		
		PrivateIncludePaths.AddRange( new string[] 
		{
			//PathFinding Paths
			/*"PathfindingLibrary/Private",
			"PathfindingLibrary/Private/CPU",*/
		});
			
		
		PublicDependencyModuleNames.AddRange( new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			
			// Texture masking for grid population
			"InputCore",
			"RenderCore",
			"RHI",
			
			//path finding
			"PathfindingLibrary",//--> this place is only for the unreal's official, commercial plugin. not for custom
		});
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
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
