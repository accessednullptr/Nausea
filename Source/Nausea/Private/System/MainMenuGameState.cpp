// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "System/MainMenuGameState.h"
#include "System/NetHelper.h"
#include "System/MapDataAsset.h"

AMainMenuGameState::AMainMenuGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

void AMainMenuGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(AMainMenuGameState, SelectedMapData, PushReplicationParams::Default);
}

bool AMainMenuGameState::SetSelectedMapData(TSoftObjectPtr<UMapDataAsset> InSelectedMapData)
{
	SelectedMapData = InSelectedMapData;
	OnRep_SelectedMapData();
	MARK_PROPERTY_DIRTY_FROM_NAME(AMainMenuGameState, SelectedMapData, this);
	return true;
}

void AMainMenuGameState::OnRep_SelectedMapData()
{
	LoadedSelectedMapData = nullptr;
	if (!SelectedMapData.IsNull() && SelectedMapData.LoadSynchronous())
	{
		LoadedSelectedMapData = SelectedMapData.Get();
	}

	OnSelectedMapDataChanged.Broadcast(this, LoadedSelectedMapData);
}