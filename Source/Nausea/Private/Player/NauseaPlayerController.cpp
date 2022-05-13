// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#include "Player/NauseaPlayerController.h"
#include "System/NauseaGameState.h"
#include "Player/NauseaPlayerState.h"
#include "Character/CoreCharacter.h"
#include "Player/NauseaPlayerCameraManager.h"

ANauseaPlayerController::ANauseaPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoManageActiveCameraTarget = false;
	PlayerCameraManagerClass = ANauseaPlayerCameraManager::StaticClass();
}

void ANauseaPlayerController::SpawnPlayerCameraManager()
{
	Super::SpawnPlayerCameraManager();

	NauseaPlayerCameraManager = Cast<ANauseaPlayerCameraManager>(PlayerCameraManager);
}

ANauseaPlayerCameraManager* ANauseaPlayerController::GetNauseaPlayerCameraManager() const
{
	return NauseaPlayerCameraManager;
}

void ANauseaPlayerController::SetIsReady(bool bIsReady)
{
	if (GetLocalRole() != ROLE_Authority)
	{
		Server_Reliable_SetIsReady(bIsReady);
		return;
	}

	ANauseaPlayerState* NauseaPlayerState = GetPlayerState<ANauseaPlayerState>();

	if (!NauseaPlayerState)
	{
		return;
	}

	NauseaPlayerState->SetIsReady(bIsReady);
}

bool ANauseaPlayerController::Server_Reliable_SetIsReady_Validate(bool bIsReady)
{
	return true;
}

void ANauseaPlayerController::Server_Reliable_SetIsReady_Implementation(bool bIsReady)
{
	SetIsReady(bIsReady);
}