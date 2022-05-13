// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Objective/WaveConfig/SimpleWaveConfig.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "System/SpawnCharacterSystem.h"
#include "Character/CoreCharacter.h"

TSubclassOf<ACoreCharacter> FSpawnGroupEntry::GetNextSpawnClass()
{
	if (LoadedSpawnMap.Num() == 0)
	{
		return nullptr;
	}

	TArray<TSubclassOf<ACoreCharacter>> ClassList;
	LoadedSpawnMap.GenerateKeyArray(ClassList);

	for (TSubclassOf<ACoreCharacter> Class : ClassList)
	{
		if (!Class)
		{
			continue;
		}

		if (!LoadedSpawnMap.Contains(Class))
		{
			continue;
		}

		LoadedSpawnMap[Class]--;
		return Class;
	}

	return nullptr;
}

void FSpawnGroupEntry::AppendSpawns(const TArray<TSubclassOf<ACoreCharacter>>& SpawnList)
{
	for (TSubclassOf<ACoreCharacter> SpawnClass : SpawnList)
	{
		if (!SpawnClass)
		{
			continue;
		}

		if (LoadedSpawnMap.Contains(SpawnClass))
		{
			LoadedSpawnMap[SpawnClass]++;
		}
		else
		{
			LoadedSpawnMap.Add(SpawnClass) = 1;
		}
	}
}

USimpleWaveConfig::USimpleWaveConfig(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void USimpleWaveConfig::Initialize()
{
	Super::Initialize();

	for (FSpawnGroupEntry& SpawnGroupEntry : SpawnGroupList)
	{
		if (!SpawnGroupEntry.SpawnGroup)
		{
			continue;
		}

		const TMap<TSoftClassPtr<ACoreCharacter>, int32>& CDOSpawnGroup = SpawnGroupEntry.SpawnGroup.GetDefaultObject()->GetSpawnGroup();

		for (const TPair<TSoftClassPtr<ACoreCharacter>, int32>& CDOEntry : CDOSpawnGroup)
		{
			if (CDOEntry.Key.IsNull())
			{
				continue;
			}

			if (TSubclassOf<ACoreCharacter> CharacterClass = CDOEntry.Key.LoadSynchronous())
			{
				SpawnGroupEntry.LoadedSpawnMap.Add(CharacterClass) = CDOEntry.Value;
			}
		}
	}
}

void USimpleWaveConfig::StartSpawning()
{
	RemainingWaitingToSpawnCount = GetTotalSpawnCount();
	GenerateSpawnGroupArray();
	Super::StartSpawning();

	if (InitialSpawnDelay > 0.f)
	{
		WaveStartTime += InitialSpawnDelay;
		GetWorld()->GetTimerManager().SetTimer(NextSpawnTimerHandle, FTimerDelegate::CreateUObject(this, &USimpleWaveConfig::RequestNextSpawn), InitialSpawnDelay, false);
	}
	else
	{
		RequestNextSpawn();
	}
}

void USimpleWaveConfig::StopSpawning()
{
	RemainingWaitingToSpawnCount = -1;
	GetWorld()->GetTimerManager().ClearTimer(NextSpawnTimerHandle);

	Super::StopSpawning();
}

void USimpleWaveConfig::SpawnedCharacterDied(UStatusComponent* Component, float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!IsActive())
	{
		return;
	}

	Super::SpawnedCharacterDied(Component, Damage, DamageEvent, EventInstigator, DamageCauser);
	UpdateWaveProgress();
}

float USimpleWaveConfig::GetTimeBetweenSpawns() const
{
	switch (SpawnMethod)
	{
		//Keep trying to spawn to ensure we stay at max concurrent.
	case ESpawnMethod::SineWave:
		break;
	default:
		return 0.1f;
	}

	const float WaveTime = GetElapsedWaveTime() / SpawnWaveFrequency;
	const float CurrentSpawnRate = 1.f + (((1.f + FMath::Sin(WaveTime * PI)) / 2.f) * SpawnRateMultiplier);

	return CurrentSpawnRate * SpawnRateBaseDelay;
}

float USimpleWaveConfig::GetNextSpawnTime() const
{
	return GetWorld() ? GetWorld()->GetTimerManager().GetTimerRemaining(NextSpawnTimerHandle) : -1.f;
}

int32 USimpleWaveConfig::GetRemainingSpawnCount() const
{
	return RemainingWaitingToSpawnCount != -1 ? RemainingWaitingToSpawnCount + GetNumberCurrentlySpawned() + PendingSpawnCount : TotalSpawnCount;
}

int32 USimpleWaveConfig::GetTotalSpawnCount() const
{
	return TotalSpawnCount;
}

int32 USimpleWaveConfig::GetDesiredSpawnCount() const
{
	int32 DesiredSpawnCount = -1;
	switch (SpawnMethod)
	{
	case ESpawnMethod::SineWave:
		DesiredSpawnCount = 4; //Calculate number of desired spawns based off of sine wave.
		break;
	case ESpawnMethod::MaintainMaxConcurrent:
	default:
		DesiredSpawnCount = MAX_int32; //Spawn the largest amount we can within defined limits.
		break;
	}

	//If a max concurrent is specified, limit it to that.
	if (MaxConcurrentSpawnCount > 0)
	{
		DesiredSpawnCount = FMath::Min(DesiredSpawnCount, FMath::Max(MaxConcurrentSpawnCount - GetNumberCurrentlySpawned(), 0));
	}

	DesiredSpawnCount -= PendingSpawnCount;
	
	//If RemainingSpawnCount is smaller than desired spawn count, use that instead.
	return FMath::Min(DesiredSpawnCount, GetRemainingSpawnCount() - GetNumberCurrentlySpawned());
}

float USimpleWaveConfig::GetWaveProgress() const
{
	return 1.f - FMath::Clamp(float(GetRemainingSpawnCount()) / float(GetTotalSpawnCount()), 0.f, 1.f);
}

FString USimpleWaveConfig::DescribeWaveConfigToGameplayDebugger() const
{
	return FString::Printf(TEXT("%s {white}Remaining Spawns: %i"), *GetName(), GetRemainingSpawnCount());
}

void USimpleWaveConfig::ProcessSpawnedCharacter(ACoreCharacter* SpawnedCharacter)
{
	PendingSpawnCount--;
	Super::ProcessSpawnedCharacter(SpawnedCharacter);
}

void USimpleWaveConfig::FailedSpawnCharacter(TSubclassOf<ACoreCharacter> CharacterClass)
{
	PendingSpawnCount--;
	Super::FailedSpawnCharacter(CharacterClass);
}

void USimpleWaveConfig::RequestNextSpawn()
{
	if (!IsActive())
	{
		return;
	}

	if (!GetOwningObjective() || GetOwningObjective()->IsPendingKill())
	{
		SetActive(false);
		return;
	}

	const int32 DesiredSpawnCount = GetDesiredSpawnCount();
	if (DesiredSpawnCount > 0)
	{
		int32 SpawnCountRemaing = DesiredSpawnCount;
		PerformSpawn(SpawnCountRemaing);
		RemainingWaitingToSpawnCount -= DesiredSpawnCount - SpawnCountRemaing;
	}

	if (GetRemainingSpawnCount() <= 0)
	{
		SetActive(false);
		return;
	}

	if (GetTimeBetweenSpawns() > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(NextSpawnTimerHandle, FTimerDelegate::CreateUObject(this, &USimpleWaveConfig::RequestNextSpawn), GetTimeBetweenSpawns(), false);
	}
}

void USimpleWaveConfig::PerformSpawn(int32& NumberOfSpawns, uint8 RetryCount)
{
	//USimpleWaveConfig::PerformSpawn is recursive on retry so we will introduce scope here to prevent anything interesting from occuring.
	{
		TArray<TSubclassOf<ACoreCharacter>> SpawnRequestList;
		FSpawnGroupEntry* NextSpawnGroup = GetNextSpawnGroup();

		uint8 SpawnGroupTryCount = 0;
		while (NextSpawnGroup)
		{
			//Try as many times as we have number of spawns requests.
			if (SpawnGroupTryCount++ > NumberOfSpawns)
			{
				break;
			}

			if (!NextSpawnGroup->IsValid())
			{
				NextSpawnGroup = GetNextSpawnGroup();
				continue;
			}

			SpawnRequestList.Add(NextSpawnGroup->GetNextSpawnClass());

			if (SpawnRequestList.Num() >= NumberOfSpawns)
			{
				break;
			}
		}

		int32 SpawnCount = GetOwningObjective()->RequestSpawn(SpawnRequestList, this);
		PendingSpawnCount += SpawnCount;
		NumberOfSpawns -= SpawnCount;

		//Push spawns that were not performed into the current NextSpawnGroup so that they can be performed later.
		if (NextSpawnGroup && SpawnRequestList.Num() != 0)
		{
			NextSpawnGroup->AppendSpawns(SpawnRequestList);
		}
	}

	//If no more spawns to try or we've tried 3 times, stop here.
	if (NumberOfSpawns == 0 || RetryCount++ > 3)
	{
		return;
	}

	PerformSpawn(NumberOfSpawns, RetryCount);
}

void USimpleWaveConfig::GenerateSpawnGroupArray()
{
	CurrentSpawnGroupList = SpawnGroupList;
}

FSpawnGroupEntry* USimpleWaveConfig::GetNextSpawnGroup()
{
	if (CurrentSpawnGroupList.Num() == 0)
	{
		GenerateSpawnGroupArray();
	}

	while (CurrentSpawnGroupList.Num() != 0)
	{
		if (CurrentSpawnGroupList[0].IsValid())
		{
			return &CurrentSpawnGroupList[0];
		}

		CurrentSpawnGroupList.RemoveAt(0, 1, false);
	}

	CurrentSpawnGroupList.Shrink();

	return nullptr;
}

void USimpleWaveConfig::UpdateWaveProgress()
{
	const float CurrentWaveProgress = GetWaveProgress();
	if (CurrentWaveProgress == LastKnownWaveProgress)
	{
		return;
	}

	LastKnownWaveProgress = CurrentWaveProgress;

	OnWaveProgressUpdate.Broadcast(this, CurrentWaveProgress);

	if (CurrentWaveProgress >= 1.f)
	{
		SpawningCompleted();
	}
}