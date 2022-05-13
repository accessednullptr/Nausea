// Copyright 1998-2019 Epic Games, Inc. Published under the MIT License.

using UnrealBuildTool;
using System.Collections.Generic;

public class NauseaServerTarget : TargetRules
{
	public NauseaServerTarget(TargetInfo Target) : base(Target)
    {
        DefaultBuildSettings = BuildSettingsVersion.V2;

        Type = TargetType.Server;
        ExtraModuleNames.Add("Nausea");
    }
}
