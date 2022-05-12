// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#include "Freerun/FreerunGameState.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameModeBase.h"
#include "Engine.h"
#include "EngineUtils.h"
#include "Engine/LevelScriptActor.h"

AFreerunGameState::AFreerunGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void AFreerunGameState::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
}

void AFreerunGameState::ResetLevel()
{
	bIsResetting = true;

	if (GetLocalRole() == ROLE_Authority)
	{
		Multicast_Reliable_ResetLevel();
	}

	//Reset controllers
	for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
	{
		AController* Controller = Iterator->Get();
		APlayerController* PlayerController = Cast<APlayerController>(Controller);
		if (PlayerController)
		{
			PlayerController->ClientReset();
		}
		Controller->Reset();
	}

	AGameModeBase* GameModeBase = GetWorld()->GetAuthGameMode();

	// Reset all actors (except controllers, the GameMode, and any other actors specified by ShouldReset())
	for (FActorIterator It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;

		if (!Actor || Actor->IsPendingKill() || Actor == this)
		{
			continue;
		}

		if (GameModeBase)
		{
			if (Actor == GameModeBase)
			{
				continue;
			}

			if (!GameModeBase->ShouldReset(Actor))
			{
				continue;
			}
		}

		Actor->Reset();
	}
	
	// Reset the GameState
	Reset();

	// Reset the GameMode
	if (GameModeBase)
	{
		GameModeBase->Reset();
	}

	// Notify the level script that the level has been reset
	ALevelScriptActor* LevelScript = GetWorld()->GetLevelScriptActor();
	if (LevelScript)
	{
		LevelScript->LevelReset();
	}

	bIsResetting = false;
}

void AFreerunGameState::Multicast_Reliable_ResetLevel_Implementation()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		return;
	}

	ResetLevel();
}