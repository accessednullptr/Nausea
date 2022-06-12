// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Overlord/DungeonGameState.h"
#include "NauseaNetDefines.h"
#include "Overlord/DungeonGameMode.h"
#include "System/CoreWorldSettings.h"
#include "System/DungeonLevelScriptActor.h"
#include "Overlord/DungeonGameModeSettings.h"

ADungeonGameState::ADungeonGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void ADungeonGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(ADungeonGameState, CurrentWaveNumber, PushReplicationParams::Default);
	DOREPLIFETIME_WITH_PARAMS_FAST(ADungeonGameState, AutoStartTime, PushReplicationParams::Default);

	DOREPLIFETIME_WITH_PARAMS_FAST(ADungeonGameState, WaveTotalSpawnCount, PushReplicationParams::Default);
	DOREPLIFETIME_WITH_PARAMS_FAST(ADungeonGameState, WaveRemainingSpawnCount, PushReplicationParams::Default);

	DOREPLIFETIME_WITH_PARAMS_FAST(ADungeonGameState, RemainingGameHealth, PushReplicationParams::Default);
}

void ADungeonGameState::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ADungeonGameState::OnRep_MatchState()
{
	if (MatchState == MatchState::EndVictory)
	{
		HandleMatchVictory();
	}
	else if (MatchState == MatchState::EndLoss)
	{
		HandleMatchLoss();
	}

	Super::OnRep_MatchState();
}

void ADungeonGameState::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
}

void ADungeonGameState::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
}

bool ADungeonGameState::IsInEndGameState() const
{
	return MatchState == MatchState::EndVictory || MatchState == MatchState::EndLoss;
}


EMatchEndType ADungeonGameState::GetEndGameStateType() const
{
	if(MatchState == MatchState::EndVictory)
	{
		return EMatchEndType::Victory;
	}
	
	if (MatchState == MatchState::EndLoss)
	{
		return EMatchEndType::Loss;
	}

	return EMatchEndType::None;
}

UDungeonWaveSetup* ADungeonGameState::GetWaveSetup() const
{
	if (CurrentWaveSetup)
	{
		return CurrentWaveSetup;
	}

	if (ADungeonLevelScriptActor* DungeonLevelScriptActor = Cast<ADungeonLevelScriptActor>(GetWorld()->GetLevelScriptActor()))
	{
		if (UDungeonWaveSetup* WaveSetup = DungeonLevelScriptActor->GenerateDungeonWaveSettings())
		{
			ensure(!CurrentWaveSetup);
			CurrentWaveSetup = WaveSetup;
			ensure(WaveSetup);
		}
	}

	return CurrentWaveSetup;
}

float ADungeonGameState::GetWaveSpawnProgress() const
{
	if (WaveTotalSpawnCount <= 0)
	{
		return 1.f;
	}

	return float(WaveRemainingSpawnCount) / float(WaveTotalSpawnCount);
}

int64 ADungeonGameState::IncrementWaveNumber()
{
	CurrentWaveNumber++;
	MARK_PROPERTY_DIRTY_FROM_NAME(ADungeonGameState, CurrentWaveNumber, this);
	OnRep_CurrentWaveNumber(CurrentWaveNumber - 1);
	return CurrentWaveNumber;
}

int64 ADungeonGameState::SetWaveTotalSpawnCount(int64 NewWaveTotalSpawnCount)
{
	const int64 PreviousWaveTotalSpawnCount = NewWaveTotalSpawnCount;
	WaveTotalSpawnCount = NewWaveTotalSpawnCount;
	MARK_PROPERTY_DIRTY_FROM_NAME(ADungeonGameState, WaveTotalSpawnCount, this);
	OnRep_WaveTotalSpawnCount(PreviousWaveTotalSpawnCount);
	SetWaveRemainingSpawnCount(NewWaveTotalSpawnCount);
	return WaveTotalSpawnCount;
}

int64 ADungeonGameState::SetWaveRemainingSpawnCount(int64 NewWaveRemainingSpawnCount)
{
	if (NewWaveRemainingSpawnCount == WaveRemainingSpawnCount)
	{
		return WaveRemainingSpawnCount;
	}

	const int64 PreviousWaveRemainingSpawnCount = WaveRemainingSpawnCount;
	WaveRemainingSpawnCount = NewWaveRemainingSpawnCount;
	MARK_PROPERTY_DIRTY_FROM_NAME(ADungeonGameState, WaveRemainingSpawnCount, this);
	OnRep_WaveRemainingSpawnCount(PreviousWaveRemainingSpawnCount);
	return WaveRemainingSpawnCount;
}

void ADungeonGameState::SetAutoStartTime(float InAutoStartTime)
{
	const float ProcessedAutoStartTime = InAutoStartTime > 0.f ? GetServerWorldTimeSeconds() + InAutoStartTime : -1.f;

	if (AutoStartTime == ProcessedAutoStartTime)
	{
		return;
	}

	const float PreviousAutoStartTime = AutoStartTime;
	AutoStartTime = ProcessedAutoStartTime;
	MARK_PROPERTY_DIRTY_FROM_NAME(ADungeonGameState, AutoStartTime, this);
	OnRep_AutoStartTime(PreviousAutoStartTime);
}

int64 ADungeonGameState::SetGameHealth(int64 Amount)
{
	if (Amount == RemainingGameHealth)
	{
		return Amount;
	}

	const int64 PreviousRemainingGameHealth = RemainingGameHealth;
	RemainingGameHealth = Amount;
	MARK_PROPERTY_DIRTY_FROM_NAME(ADungeonGameState, RemainingGameHealth, this);
	OnRep_RemainingGameHealth(PreviousRemainingGameHealth);
	return GetRemainingGameHealth();
}

int64 ADungeonGameState::DecrementGameHealth(int64 Amount)
{
	if (Amount <= 0)
	{
		return RemainingGameHealth;
	}

	SetGameHealth(FMath::Max(static_cast<int64>(0), RemainingGameHealth - Amount));
	return GetRemainingGameHealth();
}

void ADungeonGameState::HandleMatchVictory()
{
	OnGameVictory.Broadcast(this);
}

void ADungeonGameState::HandleMatchLoss()
{
	OnGameLoss.Broadcast(this);
}

void ADungeonGameState::UpdateDungeonCharacterClassList()
{
	CurrentDungeonCharacterClassList.Reset();

	UDungeonWaveSetup* WaveSetup = GetWaveSetup();

	if (!WaveSetup)
	{
		return;
	}

	WaveSetup->GetCharacterClassListForWave(CurrentWaveNumber, CurrentDungeonCharacterClassList);
}

void ADungeonGameState::OnRep_CurrentWaveNumber(int64 PreviousWaveNumber)
{
	UpdateDungeonCharacterClassList();
	OnCurrentWaveNumberChanged.Broadcast(this, CurrentWaveNumber, PreviousWaveNumber);
}


void ADungeonGameState::OnRep_WaveTotalSpawnCount(int64 PreviousWaveTotalSpawnCount)
{
	OnWaveTotalSpawnCountUpdate.Broadcast(this, WaveTotalSpawnCount, PreviousWaveTotalSpawnCount);
}

void ADungeonGameState::OnRep_WaveRemainingSpawnCount(int64 PreviousWaveRemainingSpawnCount)
{
	OnWaveRemainingSpawnCountUpdate.Broadcast(this, WaveRemainingSpawnCount, PreviousWaveRemainingSpawnCount);
}

void ADungeonGameState::OnRep_AutoStartTime(float PreviousAutoStartTime)
{
	const bool bValidTime = AutoStartTime > 0.f;
	const float TimeAdjustment = GetWorld()->GetTimeSeconds() - GetServerWorldTimeSeconds();

	const float StartTime = bValidTime ? GetWorld()->GetTimeSeconds() : -1.f;
	const float EndTime = bValidTime ? AutoStartTime + TimeAdjustment : -1.f;
	OnAutoStartTimeUpdate.Broadcast(this, StartTime, EndTime);
}

void ADungeonGameState::OnRep_RemainingGameHealth(int64 PreviousRemainingGameHealth)
{
	OnRemainingGameHeathUpdate.Broadcast(this, RemainingGameHealth, PreviousRemainingGameHealth);
}