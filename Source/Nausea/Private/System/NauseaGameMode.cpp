// Copyright 1998-2019 Epic Games, Inc. Published under the MIT License.

#include "System/NauseaGameMode.h"
#include "System/NauseaGameState.h"
#include "Player/NauseaPlayerState.h"
#include "System/NauseaLevelScriptActor.h"
#include "Objective/MissionComponent.h"

ANauseaGameMode::ANauseaGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bDelayedStart = true;
	GameStateClass = ANauseaGameState::StaticClass();
	PlayerStateClass = ANauseaPlayerState::StaticClass();
}

void ANauseaGameMode::InitGameState()
{
	Super::InitGameState();

	check(GetGameState<ANauseaGameState>());

	GetGameState<ANauseaGameState>()->OnPlayerReadyChanged.AddDynamic(this, &ANauseaGameMode::OnPlayerReadyUp);
}

void ANauseaGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

void ANauseaGameMode::StartMatch()
{
	if (HasMatchStarted())
	{
		return;
	}

	Super::StartMatch();
}

void ANauseaGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	//Should already be populated by ANauseaGameState::InitializeMissions.
	if (UMissionComponent* MissionComponent = GetGameState<ANauseaGameState>()->GetCurrentMission())
	{
		MissionComponent->SetMissionStatus(EMissionStatus::Active);
	}
}

void ANauseaGameMode::OnPlayerReadyUp_Implementation(ANauseaPlayerState* PlayerState, bool bReady)
{
	if (GetMatchState() != MatchState::WaitingToStart)
	{
		return;
	}

	ANauseaGameState* NauseaGameState = GetGameState<ANauseaGameState>();

	float NumActivePlayers = 0.f;
	float NumReadyPlayers = 0.f;

	const TArray<APlayerState*>& PlayerStateList = NauseaGameState->PlayerArray;

	for (APlayerState* ActivePlayerState : PlayerStateList)
	{
		if (!ActivePlayerState || ActivePlayerState->IsSpectator() || ActivePlayerState->IsABot())
		{
			continue;
		}

		NumActivePlayers++;

		ANauseaPlayerState* NauseaPlayerState = Cast<ANauseaPlayerState>(ActivePlayerState);

		if (!NauseaPlayerState || !NauseaPlayerState->IsReady())
		{
			continue;
		}

		NumReadyPlayers++;
	}

	if (NumReadyPlayers >= NumActivePlayers)
	{
		if (GetWorldTimerManager().IsTimerActive(ForceReadyUpTimer) && GetWorldTimerManager().GetTimerRemaining(ForceReadyUpTimer) < 2.f)
		{
			return;
		}

		GetWorldTimerManager().SetTimer(ForceReadyUpTimer, this, &ANauseaGameMode::ForceReadyCountdownTimer, 2.f, false);
		NauseaGameState->SetForceStartCountdownTime(2.f);
		return;
	}

	if (PercentReadyToStartCountdown < NumReadyPlayers / NumActivePlayers)
	{
		if (!GetWorldTimerManager().IsTimerActive(ForceReadyUpTimer))
		{
			GetWorldTimerManager().SetTimer(ForceReadyUpTimer, this, &ANauseaGameMode::ForceReadyCountdownTimer, ForceReadyUpTime, false);
			NauseaGameState->SetForceStartCountdownTime(ForceReadyUpTime);
		}
	}
	else
	{
		GetWorldTimerManager().ClearTimer(ForceReadyUpTimer);
		NauseaGameState->SetForceStartCountdownTime(-1.f);
	}
}

void ANauseaGameMode::ForceReadyCountdownTimer()
{
	StartMatch();
}