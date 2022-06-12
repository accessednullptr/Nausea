// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "System/CoreGameMode.h"
#include "DungeonGameMode.generated.h"

class ADungeonCharacter;
class UDungeonGameModeSettings;
class UDungeonWaveSetup;
class UWaveConfiguration;

namespace MatchState
{
	extern NAUSEA_API const FName EndVictory;
	extern NAUSEA_API const FName EndLoss;
}

UCLASS()
class NAUSEA_API ADungeonGameMode : public ACoreGameMode
{
	GENERATED_UCLASS_BODY()

//~ Begin AGameModeBase Interface
public:
	virtual void SetPlayerDefaults(APawn* PlayerPawn) override;
//~ End AGameModeBase Interface

//~ Begin AGameMode Interface
protected:
	virtual void HandleMatchHasStarted() override;
//~ Begin AGameMode Interface

public:
	UFUNCTION()
	int64 ProcessStartingGameHealth(int64 InStartingGameHealth);
	UFUNCTION()
	int32 ProcessStartingTrapCoinAmount(int32 InStartingTrapCoinAmount);

	UFUNCTION()
	void AddPawnToWaveCharacters(ADungeonCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = GameMode)
	static void NotifyPawnReachedEndPoint(ADungeonCharacter* Character);

	void InternalPawnReachedEndPoint(ADungeonCharacter* Character);

public:
	//ADungeonCharacter* KilledCharacter, AController* EventInstigator, AActor* DamageCauser, int32& CoinAmount
	DECLARE_EVENT_FourParams(ADungeonGameMode, FProcessCoinAmountForKillEvent, ADungeonCharacter*, AController*, AActor*, int32&)
	FProcessCoinAmountForKillEvent ProcessCoinAmountForKill;

protected:
	UFUNCTION()
	void ApplyGameDamage(int64 Amount, UObject* DamageInstigator);

	UFUNCTION()
	void OnPawnDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void OnPawnDied(UStatusComponent* Component, float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION()
	void GrantCoinsForKill(ADungeonCharacter* KilledCharacter, AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION()
	void RemovePawnFromWaveCharacters(ADungeonCharacter* Character);

	UFUNCTION()
	void OnWaveCompleted(UDungeonWaveSetup* WaveSetup, int64 WaveNumber);
	UFUNCTION()
	void StartNextWave();
	UFUNCTION()
	void PerformAutoStart(int32 AutoStartTime);

	UFUNCTION()
	void OnFinalWaveCompleted();

	UFUNCTION()
	void GoToEndGameState(bool bVictory);


protected:
	//Dump wave characters we've spawned into this list (removed on destroyed/killed) so that we know when there are no characters spawned.
	TSet<TObjectKey<ADungeonCharacter>> WaveCharacters;
	UPROPERTY(Transient)
	FTimerHandle AutoStartWaveTimerHandle;
};