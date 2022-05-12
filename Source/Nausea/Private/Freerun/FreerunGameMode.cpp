// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#include "Freerun/FreerunGameMode.h"
#include "Freerun/FreerunGameState.h"

AFreerunGameMode::AFreerunGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameStateClass = AFreerunGameState::StaticClass();
}