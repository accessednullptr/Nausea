// Copyright Epic Games, Inc. All Rights Reserved.

#include "Overlord/DungeonGameMode.h"
#include "Overlord/DungeonGameState.h"
#include "System/DungeonLevelScriptActor.h"
#include "System/CoreWorldSettings.h"
#include "Overlord/DungeonGameModeSettings.h"
#include "Player/DungeonPlayerController.h"
#include "Player/DungeonPlayerState.h"
#include "Character/DungeonCharacter.h"
#include "Overlord/TrapBase.h"
#include "Gameplay/StatusComponent.h"

namespace MatchState
{
	const FName EndVictory = FName(TEXT("EndVictory"));
	const FName EndLoss = FName(TEXT("EndLoss"));
}

ADungeonGameMode::ADungeonGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerControllerClass = ADungeonPlayerController::StaticClass();
	PlayerStateClass = ADungeonPlayerState::StaticClass();
}

void ADungeonGameMode::SetPlayerDefaults(APawn* PlayerPawn)
{
	Super::SetPlayerDefaults(PlayerPawn);
	
	if (!PlayerPawn || PlayerPawn->IsPendingKillPending())
	{
		return;
	}

	if (!PlayerPawn->OnDestroyed.IsAlreadyBound(this, &ADungeonGameMode::OnPawnDestroyed))
	{
		PlayerPawn->OnDestroyed.AddDynamic(this, &ADungeonGameMode::OnPawnDestroyed);
	}

	UStatusComponent* StatusComponent = UStatusInterfaceStatics::GetStatusComponent(PlayerPawn);

	if (StatusComponent && !StatusComponent->OnDied.IsAlreadyBound(this, &ADungeonGameMode::OnPawnDied))
	{
		StatusComponent->OnDied.AddDynamic(this, &ADungeonGameMode::OnPawnDied);
	}
}

void ADungeonGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	ADungeonGameState* DungeonGameState = GetGameState<ADungeonGameState>();
	ensure(DungeonGameState);

	int64 StartingGameHealth = 100;
	if (ACoreWorldSettings* WorldSettings = Cast<ACoreWorldSettings>(GetWorldSettings()))
	{
		if (UDungeonGameModeSettings* Settings = WorldSettings->GetGameModeSettings<UDungeonGameModeSettings>())
		{
			StartingGameHealth = Settings->CalculateStartingGameHealth(this);
		}
	}

	DungeonGameState->SetGameHealth(StartingGameHealth);

	UDungeonWaveSetup* WaveSetup = DungeonGameState->GetWaveSetup();
	ensure(WaveSetup);
	WaveSetup->OnWaveCompleted.AddDynamic(this, &ADungeonGameMode::OnWaveCompleted);

	PerformAutoStart(5);
	return;
	const int64 WaveNumber = DungeonGameState->IncrementWaveNumber();
	const int64 WaveSize = WaveSetup->InitializeWaveSetup(WaveNumber);
	WaveSetup->StartWave(WaveNumber);
	DungeonGameState->SetWaveTotalSpawnCount(WaveSize);
}

int64 ADungeonGameMode::ProcessStartingGameHealth(int64 InStartingGameHealth)
{
	return InStartingGameHealth;
}

int32 ADungeonGameMode::ProcessStartingTrapCoinAmount(int32 InStartingTrapCoinAmount)
{
	return InStartingTrapCoinAmount;
}

void ADungeonGameMode::AddPawnToWaveCharacters(ADungeonCharacter* Character)
{
	if (!Character)
	{
		return;
	}

	WaveCharacters.Add(Character);
}

void ADungeonGameMode::NotifyPawnReachedEndPoint(ADungeonCharacter* Character)
{
	if (!Character)
	{
		return;
	}

	UWorld* World = Character->GetWorld();

	if (!World)
	{
		return;
	}

	ADungeonGameMode* DungeonGameMode = World->GetAuthGameMode<ADungeonGameMode>();

	if (!DungeonGameMode)
	{
		return;
	}

	DungeonGameMode->InternalPawnReachedEndPoint(Character);
}

void ADungeonGameMode::InternalPawnReachedEndPoint(ADungeonCharacter* Character)
{
	if (!WaveCharacters.Contains(Character))
	{
		return;
	}

	const int64 DamageValue = Character->GetGameDamageValue();
	ApplyGameDamage(DamageValue, Character);

	RemovePawnFromWaveCharacters(Character);
}

void ADungeonGameMode::ApplyGameDamage(int64 Amount, UObject* DamageInstigator)
{
	ADungeonGameState* DungeonGameState = GetGameState<ADungeonGameState>();
	ensure(DungeonGameState);
	const int64 Health = DungeonGameState->DecrementGameHealth(Amount);

	if (Health <= 0 && DungeonGameState->IsMatchInProgress())
	{
		GoToEndGameState(false);
	}
}

void ADungeonGameMode::OnPawnDestroyed(AActor* DestroyedActor)
{
	if (!GetWorld() || GetWorld()->bIsTearingDown)
	{
		return;
	}
	
	if (ADungeonCharacter* DungeonCharacter = Cast<ADungeonCharacter>(DestroyedActor))
	{
		RemovePawnFromWaveCharacters(DungeonCharacter);
	}
}

void ADungeonGameMode::OnPawnDied(UStatusComponent* Component, float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!Component)
	{
		return;
	}

	ADungeonCharacter* Character = Cast<ADungeonCharacter>(Component->GetOwner());

	if (!Character)
	{
		return;
	}

	GrantCoinsForKill(Character, EventInstigator, DamageCauser);
	RemovePawnFromWaveCharacters(Character);
}

void ADungeonGameMode::GrantCoinsForKill(ADungeonCharacter* KilledCharacter, AController* EventInstigator, AActor* DamageCauser)
{
	ADungeonGameState* DungeonGameState = GetGameState<ADungeonGameState>();
	ensure(DungeonGameState);

	if (!DungeonGameState->IsMatchInProgress())
	{
		return;
	}

	const ATrapBase* TrapDamageCauser = Cast<ATrapBase>(DamageCauser);

	if (!TrapDamageCauser)
	{
		return;
	}

	ADungeonPlayerState* DungeonPlayerState = Cast<ADungeonPlayerState>(TrapDamageCauser->GetOwner());

	if (!DungeonPlayerState)
	{
		return;
	}

	int32 CoinValue = KilledCharacter->GetCoinValue();
	ProcessCoinAmountForKill.Broadcast(KilledCharacter, EventInstigator, DamageCauser, CoinValue);
	DungeonPlayerState->AddTrapCoins(CoinValue);
}

void ADungeonGameMode::RemovePawnFromWaveCharacters(ADungeonCharacter* Character)
{
	if (!WaveCharacters.Contains(Character))
	{
		return;
	}

	WaveCharacters.Remove(Character);

	ADungeonGameState* DungeonGameState = GetGameState<ADungeonGameState>();
	ensure(DungeonGameState);
	DungeonGameState->SetWaveRemainingSpawnCount(DungeonGameState->GetWaveRemainingSpawnCount() - 1);

	if (WaveCharacters.Num() != 0)
	{
		return;
	}

	DungeonGameState->SetWaveRemainingSpawnCount(0);

	if (UDungeonWaveSetup* WaveSetup = DungeonGameState->GetWaveSetup())
	{
		WaveSetup->OnWaveCharacterListEmpty();
	}
}

void ADungeonGameMode::OnWaveCompleted(UDungeonWaveSetup* WaveSetup, int64 WaveNumber)
{
	if (!WaveSetup)
	{
		return;
	}
	
	ADungeonGameState* DungeonGameState = GetGameState<ADungeonGameState>();
	ensure(DungeonGameState);
	
	if (!DungeonGameState->IsInProgress())
	{
		return;
	}

	UDungeonWaveSetup* CurrentWaveSetup = DungeonGameState->GetWaveSetup();

	if (WaveSetup != CurrentWaveSetup)
	{
		return;
	}

	if (CurrentWaveSetup->IsFinalWave(WaveNumber))
	{
		OnFinalWaveCompleted();
		return;
	}
	
	TArray<UWaveConfiguration*> WaveConfigurationList = CurrentWaveSetup->GetWaveConfiguration(WaveNumber + 1);
	if (!WaveSetup->IsWaveAutoStart(WaveConfigurationList))
	{
		return;
	}

	const int32 WaveAutoStartTime = WaveSetup->GetWaveAutoStartTime(WaveConfigurationList);
	PerformAutoStart(WaveAutoStartTime);
}

void ADungeonGameMode::StartNextWave()
{
	ADungeonGameState* DungeonGameState = GetGameState<ADungeonGameState>();
	ensure(DungeonGameState);

	if (!DungeonGameState->IsInProgress())
	{
		return;
	}

	UDungeonWaveSetup* WaveSetup = DungeonGameState->GetWaveSetup();
	ensure(WaveSetup);

	const int64 WaveNumber = DungeonGameState->IncrementWaveNumber();
	
	const int64 WaveSize = WaveSetup->InitializeWaveSetup(WaveNumber);
	DungeonGameState->SetWaveTotalSpawnCount(WaveSize);

	WaveSetup->StartWave(WaveNumber);
}

void ADungeonGameMode::PerformAutoStart(int32 AutoStartTime)
{
	ADungeonGameState* DungeonGameState = GetGameState<ADungeonGameState>();
	ensure(DungeonGameState);

	TWeakObjectPtr<ADungeonGameMode> WeakThis(this);
	GetWorld()->GetTimerManager().SetTimer(AutoStartWaveTimerHandle, FTimerDelegate::CreateWeakLambda(this, [WeakThis]()
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			ADungeonGameState* DungeonGameState = WeakThis->GetGameState<ADungeonGameState>();
			ensure(DungeonGameState);
			DungeonGameState->SetAutoStartTime(-1.f);
			WeakThis->StartNextWave();
		}), AutoStartTime, false);

	DungeonGameState->SetAutoStartTime(AutoStartTime);
}

void ADungeonGameMode::OnFinalWaveCompleted()
{
	ADungeonGameState* DungeonGameState = GetGameState<ADungeonGameState>();
	ensure(DungeonGameState);
	if (DungeonGameState->GetRemainingGameHealth() > 0 && GetMatchState() != MatchState::EndVictory)
	{
		GoToEndGameState(true);
		SetMatchState(MatchState::EndVictory);
	}
}

void ADungeonGameMode::GoToEndGameState(bool bVictory)
{
	SetMatchState(bVictory ? MatchState::EndVictory : MatchState::EndLoss);
}