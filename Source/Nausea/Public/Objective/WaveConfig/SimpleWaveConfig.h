// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Objective/ObjectiveWave.h"
#include "SimpleWaveConfig.generated.h"

class ACoreCharacter;
class UStatusComponent;

UENUM(BlueprintType)
enum class ESpawnScaleMethod : uint8
{
	Loop,
	Scale,
	MAX = 255
};

UENUM(BlueprintType)
enum class ESpawnMethod : uint8
{
	SineWave,
	MaintainMaxConcurrent,
	MAX = 255
};

USTRUCT(BlueprintType)
struct FSpawnGroupEntry
{
	GENERATED_USTRUCT_BODY()

	FSpawnGroupEntry()
	{

	}
	
public:
	bool IsValid() const { return SpawnGroup && LoadedSpawnMap.Num() > 0; }
	TSubclassOf<ACoreCharacter> GetNextSpawnClass();
	void AppendSpawns(const TArray<TSubclassOf<ACoreCharacter>>& SpawnList);

public:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<USpawnGroupObject> SpawnGroup;

	UPROPERTY(Transient, BlueprintReadOnly)
	TMap<TSubclassOf<ACoreCharacter>, int32> LoadedSpawnMap;

	static FSpawnGroupEntry InvalidGroup;
};

/**
 * 
 */
UCLASS()
class NAUSEA_API USimpleWaveConfig : public UObjectiveWaveConfig
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UObjectiveWaveConfig Interface
public:
	virtual void Initialize() override;
protected:
	virtual void StartSpawning() override;
	virtual void StopSpawning() override;
	virtual void SpawnedCharacterDied(UStatusComponent* Component, float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
public:
	virtual float GetTimeBetweenSpawns() const override;
	virtual float GetNextSpawnTime() const override;
	virtual int32 GetRemainingSpawnCount() const override;
	virtual int32 GetTotalSpawnCount() const override;
	virtual float GetWaveProgress() const override;
	virtual FString DescribeWaveConfigToGameplayDebugger() const override;
	virtual void ProcessSpawnedCharacter(ACoreCharacter* SpawnedCharacter) override;
	virtual void FailedSpawnCharacter(TSubclassOf<ACoreCharacter> CharacterClass) override;
//~ End UObjectiveWaveConfig Interface

public:
	//Returns the maximum amount of spawns this wave currently wants to perform.
	UFUNCTION(BlueprintCallable, Category = WaveConfig, BlueprintAuthorityOnly)
	virtual int32 GetDesiredSpawnCount() const;

protected:
	UFUNCTION()
	virtual void RequestNextSpawn();
	UFUNCTION()
	virtual void PerformSpawn(int32& NumberOfSpawns, uint8 RetryCount = 0);

	UFUNCTION()
	virtual void GenerateSpawnGroupArray();

	FSpawnGroupEntry* GetNextSpawnGroup();

	UFUNCTION()
	void UpdateWaveProgress();

protected:
	UPROPERTY(EditDefaultsOnly, Category = WaveConfig)
	TArray<FSpawnGroupEntry> SpawnGroupList;
	UPROPERTY(EditDefaultsOnly, Category = WaveConfig)
	int32 TotalSpawnCount = 16;
	UPROPERTY(EditDefaultsOnly, Category = WaveConfig)
	int32 MaxConcurrentSpawnCount = 8;
	UPROPERTY(EditDefaultsOnly, Category = WaveConfig)
	ESpawnScaleMethod SpawnScaleMethod = ESpawnScaleMethod::Scale;
	UPROPERTY(EditDefaultsOnly, Category = WaveConfig)
	int32 InitialSpawnDelay = 5;
	UPROPERTY(EditDefaultsOnly, Category = WaveConfig)
	ESpawnMethod SpawnMethod = ESpawnMethod::SineWave;
	UPROPERTY(EditDefaultsOnly, Category = WaveConfig)
	float SpawnRateBaseDelay = 3.f;
	UPROPERTY(EditDefaultsOnly, Category = WaveConfig)
	float SpawnRateMultiplier = 1.f;
	UPROPERTY(EditDefaultsOnly, Category = WaveConfig)
	float SpawnWaveFrequency = 100.f;

	UPROPERTY()
	TArray<FSpawnGroupEntry> CurrentSpawnGroupList;
	UPROPERTY()
	int32 RemainingWaitingToSpawnCount = -1;
	UPROPERTY(Transient)
	int32 PendingSpawnCount = 0;

	UPROPERTY()
	FTimerHandle NextSpawnTimerHandle;

	UPROPERTY()
	float LastKnownWaveProgress = -1.f;
};

UCLASS(BlueprintType, Blueprintable)
class NAUSEA_API USpawnGroupObject : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	const TMap<TSoftClassPtr<ACoreCharacter>, int32>& GetSpawnGroup() const { return SpawnGroup; }

protected:
	UPROPERTY(EditDefaultsOnly)
	TMap<TSoftClassPtr<ACoreCharacter>, int32> SpawnGroup;
};