// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Objective/WaveConfig/EndlessWaveConfig.h"


UEndlessWaveConfig::UEndlessWaveConfig(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TotalSpawnCount = -1;
}

void UEndlessWaveConfig::GenerateSpawnGroupArray()
{
	if (bUseSpecialSpawnGroupWhenFinishedSpawnGroupList)
	{
		if (bFirstSpawnGroupListUse)
		{
			Super::GenerateSpawnGroupArray();
			bFirstSpawnGroupListUse = false;
			return;
		}

		CurrentSpawnGroupList = SpecialSpawnGroupList;
		return;
	}

	Super::GenerateSpawnGroupArray();
}