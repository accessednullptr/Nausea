// Copyright 1998-2019 Epic Games, Inc. Published under the MIT License.

using UnrealBuildTool;

public class NauseaEditor : ModuleRules
{
	public NauseaEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        DefaultBuildSettings = BuildSettingsVersion.V2;

        PublicIncludePaths.AddRange( new string[] { "NauseaEditor", "NauseaEditor/Public" });
        PrivateIncludePaths.AddRange(new string[] { "NauseaEditor/Private" });

        PublicDependencyModuleNames.AddRange( new string[] { "Nausea" });

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "NetCore", "PhysicsCore", "InputCore", "AIModule", "NavigationSystem", "SlateCore", "Slate", "UMG", "GameplayAbilities", "GameplayTags", "GameplayTasks", "GameplayDebugger", "EngineSettings" });
        
        if (Target.Type == TargetType.Editor || Target.bBuildEditor == true)
        {
            PublicDependencyModuleNames.AddRange(new string[] { "RenderCore", "RHI", "UnrealEd" });
        }
    }
}
