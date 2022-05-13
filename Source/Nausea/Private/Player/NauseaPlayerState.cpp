// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Player/NauseaPlayerState.h"
#include "System/NetHelper.h"
#include "Player/PlayerOwnershipInterfaceTypes.h"

ANauseaPlayerState::ANauseaPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void ANauseaPlayerState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(ANauseaPlayerState, bIsReady, PushReplicationParams::Default);
}

void ANauseaPlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();

	if (GetPlayerName() != "")
	{
		OnPlayerNameChanged.Broadcast(this, GetPlayerName());
	}
}

void ANauseaPlayerState::SetIsReady(bool bInIsReady)
{
	if (bIsReady == bInIsReady || GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	bIsReady = bInIsReady;
	OnRep_IsReady();
	MARK_PROPERTY_DIRTY_FROM_NAME(ANauseaPlayerState, bIsReady, this);
	ForceNetUpdate();
}

void ANauseaPlayerState::OnRep_IsReady()
{
	OnReadyChanged.Broadcast(this, IsReady());
}