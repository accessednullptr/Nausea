// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "DungeonLevelScriptActor.generated.h"

class UDungeonWaveSetup;

/**
 * 
 */
UCLASS()
class NAUSEA_API ADungeonLevelScriptActor : public ALevelScriptActor
{
	GENERATED_UCLASS_BODY()
	

public:
	UDungeonWaveSetup* GenerateDungeonWaveSettings();

protected:
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, Category = WaveSettings, meta = (DisplayName="Generate Dungeon Wave Settings", ScriptName="GenerateDungeonWaveSettings"))
	UDungeonWaveSetup* K2_GenerateDungeonWaveSettings();

protected:
	UPROPERTY(Transient)
	UDungeonWaveSetup* GeneratedDungeonWaveSettings = nullptr;
};
