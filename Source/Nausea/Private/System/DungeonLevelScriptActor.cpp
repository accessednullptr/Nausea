// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "System/DungeonLevelScriptActor.h"

ADungeonLevelScriptActor::ADungeonLevelScriptActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

UDungeonWaveSetup* ADungeonLevelScriptActor::GenerateDungeonWaveSettings()
{
	if (GeneratedDungeonWaveSettings)
	{
		return GeneratedDungeonWaveSettings;
	}

	GeneratedDungeonWaveSettings = K2_GenerateDungeonWaveSettings();
	return GeneratedDungeonWaveSettings;
}