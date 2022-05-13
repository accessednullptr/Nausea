// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "System/MainMenuGameMode.h"
#include "GameFramework/PlayerController.h"

AMainMenuGameMode::AMainMenuGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void AMainMenuGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	if (!IsMatchInProgress())
	{
		return;
	}

	if (bStartPlayersAsSpectators || MustSpectate(NewPlayer))
	{
		return;
	}

	if (!NewPlayer)
	{
		return;
	}

	if (!PlayerCanRestart(NewPlayer))
	{
		GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &AMainMenuGameMode::RetryHandleStartingNewPlayer, TWeakObjectPtr<APlayerController>(NewPlayer)));
		return;
	}

	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

FString AMainMenuGameMode::GenerateLevelTravelURL() const
{
	return FString("");
}

void AMainMenuGameMode::RetryHandleStartingNewPlayer(TWeakObjectPtr<APlayerController> PlayerController)
{
	HandleStartingNewPlayer(PlayerController.Get());
}