// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Player/DungeonPlayerState.h"
#include "NauseaNetDefines.h"
#include "System/CoreWorldSettings.h"
#include "Overlord/DungeonGameMode.h"
#include "Overlord/DungeonGameModeSettings.h"

ADungeonPlayerState::ADungeonPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

void ADungeonPlayerState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(ADungeonPlayerState, TrapCoins, PushReplicationParams::Default);
}

void ADungeonPlayerState::BeginPlay()
{
	Super::BeginPlay();
	
	if (ACoreWorldSettings* WorldSettings = Cast<ACoreWorldSettings>(GetWorldSettings()))
	{
		if (UDungeonGameModeSettings* GameModeSettings = WorldSettings->GetGameModeSettings<UDungeonGameModeSettings>())
		{
			SetTrapCoins(GameModeSettings->CalculateStartingTrapCoinAmount(GetWorld()->GetAuthGameMode<ADungeonGameMode>()));
		}
	}
}

int32 ADungeonPlayerState::GetTrapCoins() const
{
	return TrapCoins;
}

int32 ADungeonPlayerState::AddTrapCoins(int32 Amount)
{
	if (Amount <= 0)
	{
		return TrapCoins;
	}

	TrapCoins += Amount;
	OnRep_TrapCoins();
	return TrapCoins;
}

int32 ADungeonPlayerState::RemoveTrapCoins(int32 Amount)
{
	if (Amount <= 0)
	{
		return TrapCoins;
	}

	TrapCoins -= Amount;
	TrapCoins = FMath::Max(0, TrapCoins);
	OnRep_TrapCoins();
	return TrapCoins;
}

int32 ADungeonPlayerState::SetTrapCoins(int32 Amount)
{
	TrapCoins = FMath::Max(0, Amount);
	OnRep_TrapCoins();
	return TrapCoins;
}

bool ADungeonPlayerState::HasEnoughTrapCoins(int32 InCost) const
{
	return GetTrapCoins() >= InCost;
}

void ADungeonPlayerState::OnRep_TrapCoins()
{
	OnTrapCoinsChanged.Broadcast(this, GetTrapCoins());
}