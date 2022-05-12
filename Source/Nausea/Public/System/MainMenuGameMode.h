// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "System/CoreGameMode.h"
#include "MainMenuGameMode.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API AMainMenuGameMode : public ACoreGameMode
{
	GENERATED_UCLASS_BODY()

//~ Begin AGameModeBase Interface
public:
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer);
//~ End AGameModeBase Interface

public:
	virtual FString GenerateLevelTravelURL() const;

protected:
	void RetryHandleStartingNewPlayer(TWeakObjectPtr<APlayerController> PlayerController);
};
