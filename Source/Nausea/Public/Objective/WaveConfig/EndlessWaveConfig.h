// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Objective/WaveConfig/SimpleWaveConfig.h"
#include "EndlessWaveConfig.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API UEndlessWaveConfig : public USimpleWaveConfig
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UObjectiveWaveConfig Interface
public:
	virtual int32 GetRemainingSpawnCount() const override { return MAX_int32 / 2; }
	virtual int32 GetTotalSpawnCount() const override { return MAX_int32 / 2; }
	virtual float GetWaveProgress() const override { return 0.f; }
//~ End UObjectiveWaveConfig Interface

//~ Begin USimpleWaveConfig Interface
public:
	virtual void GenerateSpawnGroupArray() override;
//~ End USimpleWaveConfig Interface

protected:
	UPROPERTY(EditDefaultsOnly, Category = WaveConfig)
	bool bUseSpecialSpawnGroupWhenFinishedSpawnGroupList = false;

	UPROPERTY(EditDefaultsOnly, Category = WaveConfig, meta = (EditCondition = "bUseSpecialSpawnGroupWhenFinishedSpawnGroupList"))
	TArray<FSpawnGroupEntry> SpecialSpawnGroupList;

	UPROPERTY()
	bool bFirstSpawnGroupListUse = true;
};
