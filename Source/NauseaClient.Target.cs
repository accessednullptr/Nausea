// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class NauseaClientTarget : TargetRules
{
	public NauseaClientTarget(TargetInfo Target) : base(Target)
    {
        DefaultBuildSettings = BuildSettingsVersion.V2;

        Type = TargetType.Client;
        ExtraModuleNames.Add("Nausea");
    }
}
