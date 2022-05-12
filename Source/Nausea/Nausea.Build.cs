// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Nausea : ModuleRules
{
	public Nausea(ReadOnlyTargetRules Target) : base(Target)
    {
        DefaultBuildSettings = BuildSettingsVersion.V2;

        PublicIncludePaths.AddRange(new string[] { "Nausea", "Nausea/Public" });
        PrivateIncludePaths.AddRange(new string[] { "Nausea/Private" });

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "NetCore", "PhysicsCore", "InputCore", "AIModule", "NavigationSystem", "SlateCore", "Slate", "UMG", "GameplayAbilities", "GameplayTags", "GameplayTasks", "GameplayDebugger", "EngineSettings"});

        if (Target.Type == TargetType.Editor)
        {
            PublicDependencyModuleNames.AddRange(new string[] { "RenderCore", "RHI", "UnrealEd" });
        }
    }
}
