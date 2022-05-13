// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#include "Freerun/FreerunGameMode.h"
#include "Freerun/FreerunGameState.h"

AFreerunGameMode::AFreerunGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameStateClass = AFreerunGameState::StaticClass();
}