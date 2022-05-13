// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Objective/WaveConfig/MultiWaveConfig.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "Internationalization/StringTableRegistry.h"
#include "GameFramework/GameStateBase.h"

UMultiWaveConfig::UMultiWaveConfig(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ObjectiveNameOverride = LOCTABLE("/Game/Localization/ObjectiveStringTable.ObjectiveStringTable", "Objective_Wave_Multi");
}

void UMultiWaveConfig::Initialize()
{
	Super::Initialize();

	TArray<TSubclassOf<UObjectiveWaveConfig>> ProcessedWaveConfigClassList;
	for (TSubclassOf<UObjectiveWaveConfig> WaveConfigClass : WaveConfigClassList)
	{
		UMultiWaveConfig* PotentialMultiWaveConfigCDO = Cast<UMultiWaveConfig>(WaveConfigClass.GetDefaultObject());
		if (!PotentialMultiWaveConfigCDO)
		{
			ProcessedWaveConfigClassList.Add(WaveConfigClass);
			continue;
		}

		ProcessedWaveConfigClassList.Append(PotentialMultiWaveConfigCDO->WaveConfigClassList);
	}

	WaveInstanceList.Reserve(ProcessedWaveConfigClassList.Num());
	for (TSubclassOf<UObjectiveWaveConfig> WaveConfigClass : ProcessedWaveConfigClassList)
	{
		if (!WaveConfigClass)
		{
			continue;
		}

		WaveInstanceList.Add(NewObject<UObjectiveWaveConfig>(this, WaveConfigClass));
	}
	WaveInstanceList.Shrink();

	for (UObjectiveWaveConfig* WaveInstance : WaveInstanceList)
	{
		if (!WaveInstance)
		{
			continue;
		}

		WaveInstance->Initialize();
	}

	GetOwningObjective()->SetCurrentWave(GetCurrentWave());
	GetOwningObjective()->SetTotalWaveCount(GetTotalWaveCount());
	WaveCountUpdated();
}

void UMultiWaveConfig::StartSpawning()
{
	SetCurrentWaveIndex(0);
	StartNextWave();
}

void UMultiWaveConfig::StopSpawning()
{
	if (ActiveWaveConfigInstance && ActiveWaveConfigInstance->IsActive())
	{
		//Unbind ahead of time so that we do not receive events from shutdown.
		UnbindToActiveWaveInstance();
		ActiveWaveConfigInstance->SetActive(false);
	}

	SetWaveStatus(EWaveStatus::Completed);
	SetCurrentWaveIndex(GetTotalWaveCount() - 1);
	ActiveWaveConfigInstance = nullptr;
}

FString UMultiWaveConfig::DescribeWaveConfigToGameplayDebugger() const
{
	if (ActiveWaveConfigInstance)
	{
		FString ActiveWaveConfigDescription = ActiveWaveConfigInstance->DescribeWaveConfigToGameplayDebugger();

		if (!ActiveWaveConfigDescription.IsEmpty())
		{
			return FString::Printf(TEXT("%s %s\n            {yellow}%s"),
				*GetName(),
				*FString::Printf(TEXT("{white}Wave %i / %i"), GetCurrentWave(), GetTotalWaveCount()),
				*ActiveWaveConfigDescription);
		}
	}

	return FString::Printf(TEXT("%s {white}Wave: %i / %i"), *GetName(), GetCurrentWave(), GetTotalWaveCount());
}

void UMultiWaveConfig::StartNextWave()
{
	//If we somehow hit an invalid wave index, exit this wave config.
	if (!WaveInstanceList.IsValidIndex(GetCurrentWaveIndex()))
	{
		SpawningCompleted();
		return;
	}

	OnWaveProgressUpdate.Broadcast(this, 0.f);

	//Seek next valid wave instance.
	while (!WaveInstanceList[GetCurrentWaveIndex()])
	{
		SetCurrentWaveIndex(CurrentWaveIndex + 1);

		if (!WaveInstanceList.IsValidIndex(GetCurrentWaveIndex()))
		{
			SpawningCompleted();
			return;
		}
	}

	ActiveWaveConfigInstance = WaveInstanceList[GetCurrentWaveIndex()];
	ActiveWaveConfigInstance->SetActive(true);
	BindToActiveWaveInstance();

	SetWaveStatus(EWaveStatus::InProgress);
}

void UMultiWaveConfig::OnChildWaveCompleted(UObjectiveWaveConfig* Wave)
{
	if (ActiveWaveConfigInstance && ActiveWaveConfigInstance->IsActive())
	{
		UnbindToActiveWaveInstance();
		ActiveWaveConfigInstance->SetActive(false);
		ActiveWaveConfigInstance = nullptr;
	}

	//If the next wave index is going to be invalid, then mark this multiwave as complete and early out.
	if (!WaveInstanceList.IsValidIndex(GetCurrentWaveIndex() + 1))
	{
		SpawningCompleted();
		return;
	}

	if (bIntermissionBetweenWaves)
	{
		StartWaveIntermission();
		
		return;
	}
}

void UMultiWaveConfig::OnChildWaveProgressUpdate(UObjectiveWaveConfig* Wave, float Progress)
{
	if (Wave != ActiveWaveConfigInstance)
	{
		return;
	}

	OnWaveProgressUpdate.Broadcast(this, Progress);
}

void UMultiWaveConfig::BindToActiveWaveInstance()
{
	if (!ActiveWaveConfigInstance || ActiveWaveConfigInstance->OnWaveCompleted.IsAlreadyBound(this, &UMultiWaveConfig::OnChildWaveCompleted))
	{
		return;
	}

	ActiveWaveConfigInstance->OnWaveCompleted.AddDynamic(this, &UMultiWaveConfig::OnChildWaveCompleted);
	ActiveWaveConfigInstance->OnWaveProgressUpdate.AddDynamic(this, &UMultiWaveConfig::OnChildWaveProgressUpdate);
}

void UMultiWaveConfig::UnbindToActiveWaveInstance()
{
	if (!ActiveWaveConfigInstance || !ActiveWaveConfigInstance->OnWaveCompleted.IsAlreadyBound(this, &UMultiWaveConfig::OnChildWaveCompleted))
	{
		return;
	}

	ActiveWaveConfigInstance->OnWaveCompleted.RemoveDynamic(this, &UMultiWaveConfig::OnChildWaveCompleted);
	ActiveWaveConfigInstance->OnWaveProgressUpdate.RemoveDynamic(this, &UMultiWaveConfig::OnChildWaveProgressUpdate);
}

void UMultiWaveConfig::SetCurrentWaveIndex(int32 InCurrentWave)
{
	if (CurrentWaveIndex == InCurrentWave)
	{
		return;
	}

	CurrentWaveIndex = InCurrentWave;
	OnWaveConfigCurrentWaveChanged.Broadcast(this, GetCurrentWave());
}

void UMultiWaveConfig::WaveCountUpdated()
{
	OnWaveConfigTotalWaveCountChanged.Broadcast(this, GetTotalWaveCount());
}

void UMultiWaveConfig::StartWaveIntermission()
{
	SetWaveStatus(EWaveStatus::Intermission);

	if (WaveIntermissionDuration > 0.f)
	{
		const float WorldTimeSeconds = GetWorld()->GetGameState<AGameStateBase>()->GetServerWorldTimeSeconds();

		if (UObjectiveWave* Objective = GetOwningObjective())
		{
			Objective->SetIntermissionTime(FVector2D(WorldTimeSeconds, WorldTimeSeconds + WaveIntermissionDuration));
		}

		GetWorld()->GetTimerManager().SetTimer(WaveIntermissionTimer, FTimerDelegate::CreateUObject(this, &UMultiWaveConfig::WaveIntermissionExpired), WaveIntermissionDuration, false);
	}

	const int32 PreviousWaveIndex = GetCurrentWaveIndex() - 1;
	OnStartWaveIntermission(WaveInstanceList.IsValidIndex(PreviousWaveIndex) ? WaveInstanceList[PreviousWaveIndex] : nullptr);
}

void UMultiWaveConfig::ManuallyEndWaveIntermission()
{
	WaveIntermissionExpired();
}

void UMultiWaveConfig::WaveIntermissionExpired()
{
	SetCurrentWaveIndex(CurrentWaveIndex + 1);
	OnWaveIntermissionExpired(WaveInstanceList.IsValidIndex(GetCurrentWaveIndex()) ? WaveInstanceList[GetCurrentWaveIndex()] : nullptr);
	StartNextWave();
}