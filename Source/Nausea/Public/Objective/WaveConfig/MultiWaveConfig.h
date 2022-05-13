// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Objective/ObjectiveWave.h"
#include "MultiWaveConfig.generated.h"



/**
 * 
 */
UCLASS()
class NAUSEA_API UMultiWaveConfig : public UObjectiveWaveConfig
{
	GENERATED_UCLASS_BODY()

//~ Begin UObjectiveWaveConfig Interface
public:
	virtual void Initialize() override;
protected:
	virtual void StartSpawning() override;
	virtual void StopSpawning() override;

#define PASS_OVERRIDE_TO_ACTIVE_WAVE(FunctionName, NullResult) if(ActiveWaveConfigInstance && ActiveWaveConfigInstance->IsActive()) { return ActiveWaveConfigInstance->FunctionName(); } return NullResult;\

public:
	virtual float GetTimeBetweenSpawns() const override { PASS_OVERRIDE_TO_ACTIVE_WAVE(GetTimeBetweenSpawns, -1.f) }
	virtual float GetNextSpawnTime() const override { PASS_OVERRIDE_TO_ACTIVE_WAVE(GetNextSpawnTime, -1.f) }
	virtual int32 GetRemainingSpawnCount() const override { PASS_OVERRIDE_TO_ACTIVE_WAVE(GetRemainingSpawnCount, -1) }
	virtual int32 GetTotalSpawnCount() const override { PASS_OVERRIDE_TO_ACTIVE_WAVE(GetTotalSpawnCount, -1) }
	virtual float GetWaveProgress() const override { PASS_OVERRIDE_TO_ACTIVE_WAVE(GetWaveProgress, -1.f) }

	virtual int32 GetCurrentWave() const { return GetCurrentWaveIndex() + 1; }
	virtual int32 GetTotalWaveCount() const { return WaveInstanceList.Num(); }
	virtual FString DescribeWaveConfigToGameplayDebugger() const;
//~ End UObjectiveWaveConfig Interface

public:
	UFUNCTION(BlueprintCallable, Category = Objective, BlueprintAuthorityOnly)
	int32 GetCurrentWaveIndex() const { return CurrentWaveIndex; }

protected:
	UFUNCTION()
	void StartNextWave();
	UFUNCTION()
	void OnChildWaveCompleted(UObjectiveWaveConfig* Wave);
	UFUNCTION()
	void OnChildWaveProgressUpdate(UObjectiveWaveConfig* Wave, float Progress);

	UFUNCTION()
	void BindToActiveWaveInstance();
	UFUNCTION()
	void UnbindToActiveWaveInstance();

	UFUNCTION()
	void SetCurrentWaveIndex(int32 InCurrentWave);
	UFUNCTION()
	void WaveCountUpdated();

	UFUNCTION()
	void StartWaveIntermission();
	UFUNCTION(BlueprintImplementableEvent, Category = Objective, BlueprintAuthorityOnly)
	void OnStartWaveIntermission(UObjectiveWaveConfig* CompletedWave);

	UFUNCTION(BlueprintCallable, Category = Objective, BlueprintAuthorityOnly)
	void ManuallyEndWaveIntermission();
	UFUNCTION()
	void WaveIntermissionExpired();
	UFUNCTION(BlueprintImplementableEvent, Category = Objective, BlueprintAuthorityOnly)
	void OnWaveIntermissionExpired(UObjectiveWaveConfig* IncomingWave);

protected:
	UPROPERTY(EditDefaultsOnly, Category = WaveConfig)
	bool bIntermissionBetweenWaves = true;

	//If bIntermissionBetweenWaves is true and this is a number less than 0, the intermission must be manually controlled via 
	UPROPERTY(EditDefaultsOnly, Category = WaveConfig)
	float WaveIntermissionDuration = 40.f;
	UPROPERTY()
	FTimerHandle WaveIntermissionTimer;

	UPROPERTY(EditDefaultsOnly, Category = WaveConfig)
	TArray<TSubclassOf<UObjectiveWaveConfig>> WaveConfigClassList;

	UPROPERTY()
	UObjectiveWaveConfig* ActiveWaveConfigInstance = nullptr;
	UPROPERTY()
	TArray<UObjectiveWaveConfig*> WaveInstanceList;
	UPROPERTY()
	int32 CurrentWaveIndex = -1;
};