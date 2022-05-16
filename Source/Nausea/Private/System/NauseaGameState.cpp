// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#include "System/NauseaGameState.h"
#include "NauseaNetDefines.h"
#include "System/NauseaLevelScriptActor.h"
#include "Player/NauseaPlayerState.h"
#include "Player/PlayerClassComponent.h"
#include "Objective/MissionComponent.h"

ANauseaGameState::ANauseaGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void ANauseaGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(ANauseaGameState, CurrentMission, PushReplicationParams::Default);
	DOREPLIFETIME_WITH_PARAMS_FAST(ANauseaGameState, InitialMission, PushReplicationParams::InitialOnly);

	DOREPLIFETIME_WITH_PARAMS_FAST(ANauseaGameState, ForceStartCountdownTime, PushReplicationParams::Default);
}

void ANauseaGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

	ANauseaPlayerState* NauseaPlayerState = Cast<ANauseaPlayerState>(PlayerState);

	if (!NauseaPlayerState)
	{
		return;
	}

	if (NauseaPlayerState->IsABot())
	{
		return;
	}

	if (!NauseaPlayerState->OnReadyChanged.IsAlreadyBound(this, &ANauseaGameState::OnPlayerReadyUpUpdate))
	{
		NauseaPlayerState->OnReadyChanged.AddDynamic(this, &ANauseaGameState::OnPlayerReadyUpUpdate);
	}
}

void ANauseaGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);

	ANauseaPlayerState* NauseaPlayerState = Cast<ANauseaPlayerState>(PlayerState);

	if (!NauseaPlayerState)
	{
		return;
	}

	if (NauseaPlayerState->IsABot())
	{
		return;
	}

	NauseaPlayerState->OnReadyChanged.RemoveDynamic(this, &ANauseaGameState::OnPlayerReadyUpUpdate);
}

void ANauseaGameState::InitializeGameState(ACoreGameMode* CoreGameMode)
{
	Super::InitializeGameState(CoreGameMode);

	InitializeMissions();

	//Load all classes specified into memory.
	PlayerClassList.Reserve(DefaultPlayerClassList.Num());
	for (TSoftClassPtr<UPlayerClassComponent> SoftPlayerClass : DefaultPlayerClassList)
	{
		TSubclassOf<UPlayerClassComponent> PlayerClass = TSubclassOf<UPlayerClassComponent>(SoftPlayerClass.LoadSynchronous());

		if (!PlayerClass)
		{
			continue;
		}

		PlayerClassList.Add(PlayerClass);
	}
	PlayerClassList.Shrink();

	MARK_PROPERTY_DIRTY_FROM_NAME(ACoreGameState, PlayerClassList, this);
}

void ANauseaGameState::InitializeMissions()
{
	ANauseaLevelScriptActor* NauseaLevelScriptActor = Cast<ANauseaLevelScriptActor>(GetWorld()->GetLevelScriptActor());

	if (!NauseaLevelScriptActor)
	{
		return;
	}

	SetCurrentMission(NauseaLevelScriptActor->GetFirstMission(), false);
}

void ANauseaGameState::SetCurrentMission(UMissionComponent* InMission, bool bAutoActivate)
{
	if (GetCurrentMission() == InMission)
	{
		return;
	}

	UMissionComponent* PreviousMission = CurrentMission;
	CurrentMission = InMission;

	if (bAutoActivate && GetCurrentMission())
	{
		GetCurrentMission()->SetMissionStatus(EMissionStatus::Active);
	}

	if (!InitialMission)
	{
		InitialMission = CurrentMission;
		MARK_PROPERTY_DIRTY_FROM_NAME(ANauseaGameState, InitialMission, this);
	}

	OnRep_CurrentMission(PreviousMission);
	MARK_PROPERTY_DIRTY_FROM_NAME(ANauseaGameState, CurrentMission, this);
}

void ANauseaGameState::SetForceStartCountdownTime(float InCountdownDuration)
{
	if (InCountdownDuration <= 0.f)
	{
		ForceStartCountdownTime = FVector2D(-1.f);
		OnRep_ForceStartCountdownTime();
		MARK_PROPERTY_DIRTY_FROM_NAME(ANauseaGameState, ForceStartCountdownTime, this);
		return;
	}

	ForceStartCountdownTime.X = GetServerWorldTimeSeconds();
	ForceStartCountdownTime.Y = GetServerWorldTimeSeconds() + InCountdownDuration;
	OnRep_ForceStartCountdownTime();
	MARK_PROPERTY_DIRTY_FROM_NAME(ANauseaGameState, ForceStartCountdownTime, this);
}

float ANauseaGameState::GetCountdownPercentRemaining() const
{
	if (ForceStartCountdownTime.X <= 0.f)
	{
		return 0.f;
	}

	return FMath::Max(0.f, 1.f - (GetServerWorldTimeSeconds() - ForceStartCountdownTime.X) / (ForceStartCountdownTime.Y - ForceStartCountdownTime.X));
}

void ANauseaGameState::OnRep_CurrentMission(UMissionComponent* PreviousMission)
{
	if (PreviousMission && PreviousMission->OnMissionCompleted.IsAlreadyBound(this, &ANauseaGameState::MissionCompleted))
	{
		PreviousMission->OnMissionCompleted.RemoveDynamic(this, &ANauseaGameState::MissionCompleted);
		PreviousMission->OnMissionFailed.RemoveDynamic(this, &ANauseaGameState::MissionFailed);
	}

	if (GetCurrentMission() && !GetCurrentMission()->OnMissionCompleted.IsAlreadyBound(this, &ANauseaGameState::MissionCompleted))
	{
		GetCurrentMission()->OnMissionCompleted.AddDynamic(this, &ANauseaGameState::MissionCompleted);
		GetCurrentMission()->OnMissionFailed.AddDynamic(this, &ANauseaGameState::MissionFailed);
	}

	OnMissionChanged.Broadcast(GetCurrentMission(), PreviousMission);
}

void ANauseaGameState::MissionCompleted(UMissionComponent* Mission)
{
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (!Mission || Mission != GetCurrentMission())
	{
		return;
	}

	SetCurrentMission(Mission->GetFollowingMission());
}

void ANauseaGameState::MissionFailed(UMissionComponent* Mission)
{

}

void ANauseaGameState::OnPlayerReadyUpUpdate(ANauseaPlayerState* PlayerState, bool bReady)
{
	OnPlayerReadyChanged.Broadcast(PlayerState, bReady);
}

void ANauseaGameState::OnRep_ForceStartCountdownTime()
{
	if (ForceStartCountdownTime == FVector2D(-1.f))
	{
		OnForceStartCountdownStateChanged.Broadcast(this, false);
	}
	else
	{
		OnForceStartCountdownStateChanged.Broadcast(this, true);
	}
}

ANauseaGameState* ANauseaGameState::GetNauseaGameState(const UObject* WorldContextObject)
{
	UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	return World ? World->GetGameState<ANauseaGameState>() : nullptr;
}