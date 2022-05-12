// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "System/CoreGameMode.h"
#include "NauseaGameMode.generated.h"

class ANauseaPlayerState;

UCLASS(minimalapi)
class ANauseaGameMode : public ACoreGameMode
{
	GENERATED_UCLASS_BODY()

//~ Begin AGameModeBase Interface
public:
	virtual void InitGameState() override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
//~ End AGameModeBase Interface
	
//~ Begin AGameMode Interface
public:
	virtual void StartMatch() override;
protected:
	virtual void HandleMatchHasStarted() override;
//~ End AGameMode Interface

protected:
	//Handles players reading up during the start of the match or mid match.
	UFUNCTION(BlueprintNativeEvent, Category = GameMode)
	void OnPlayerReadyUp(ANauseaPlayerState* PlayerState, bool bReady);

	UFUNCTION()
	void ForceReadyCountdownTimer();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GameMode)
	float PercentReadyToStartCountdown = 0.667f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GameMode)
	float ForceReadyUpTime = 30.f;
	UPROPERTY()
	FTimerHandle ForceReadyUpTimer;
};