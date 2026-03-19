// Copyright Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;
public class ProjectER : ModuleRules
{
    public ProjectER(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "AIModule",
            "NavigationSystem",
            "StateTreeModule",
            "GameplayStateTreeModule",
            "UMG",
            "Slate",
            "SlateCore",
            "GameplayAbilities",
            "GameplayTags",
            "GameplayTasks",
            "Niagara",
            "HexGridPlugin",
            "PathFindingLibrary",
            "WorldBender",
            "TopDownVision",
            "AssetRegistry",
            "OnlineSubsystemSteam",
            "OnlineSubsystem",
            "OnlineSubsystemUtils",
            "PhysicsCore",// for physic material
            //"UnrealEd",//for the editor function
            "Projects",// for plugin module (IPluginManager::Get())

        });
        
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "RenderCore",//for shader module to be used
        });
        
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                // for editor
                "UnrealEd", 
            });
        }
        
        PublicIncludePaths.AddRange(new string[]
        {
            "ProjectER"
        });
        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");
        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
