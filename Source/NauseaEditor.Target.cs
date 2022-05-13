// Copyright 1998-2019 Epic Games, Inc. Published under the MIT License.

using UnrealBuildTool;
using System.Collections.Generic;

public class NauseaEditorTarget : TargetRules
{
	public NauseaEditorTarget(TargetInfo Target) : base(Target)
    {
        DefaultBuildSettings = BuildSettingsVersion.V2;

        Type = TargetType.Editor;
        ExtraModuleNames.Add("Nausea");
        ExtraModuleNames.Add("NauseaEditor");
    }
}
