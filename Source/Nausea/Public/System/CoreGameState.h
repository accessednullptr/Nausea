// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "CoreGameState.generated.h"

class ACoreGameMode;
class ACorePlayerState;
class USpawnCharacterSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMatchStateChanged, ACoreGameState*, GameState, FName, MatchState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerArrayChangeSignature, bool, bIsPlayer, ACorePlayerState*, PlayerState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerStateRelevancyChangeSignature, ACorePlayerState*, PlayerState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerSpectatorChangedSignature, ACorePlayerState*, PlayerState, bool, bIsSpectator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerAliveChangedSignature, ACorePlayerState*, PlayerState, bool, bAlive);

/**
 * 
 */
UCLASS()
class NAUSEA_API ACoreGameState : public AGameState
{
	GENERATED_UCLASS_BODY()
	
//~ Begin AGameStateBase Interface
protected:
	virtual void BeginPlay() override;
public:
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	virtual void OnRep_MatchState() override;
//~ End AGameStateBase Interface

public:
	virtual void InitializeGameState(ACoreGameMode* CoreGameMode);
	virtual void OnPlayerStateReady(ACorePlayerState* PlayerState);

	UFUNCTION(BlueprintCallable, Category = GameState)
	bool IsWaitingToStart() const;
	UFUNCTION(BlueprintCallable, Category = GameState)
	bool IsInProgress() const;
	UFUNCTION(BlueprintCallable, Category = GameState)
	bool IsWaitingPostMatch() const;

	UFUNCTION(BlueprintCallable, Category = GameState)
	const TArray<TSubclassOf<UPlayerClassComponent>>& GetPlayerClassList() const { return PlayerClassList; }
	//Returns true if a given player can EVER select this class.
	UFUNCTION()
	virtual bool CanPlayerSelectPlayerClass(const ACorePlayerController* PlayerController, TSubclassOf<UPlayerClassComponent> PlayerClass) const;

	UFUNCTION()
	virtual void HandleDamageApplied(AActor* Actor, float& DamageAmount, FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION()
	virtual void HandleStatusApplied(AActor* Actor, float& EffectPower, FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, Category = GameState)
	bool AreThereAnyAlivePlayers() const;

	UFUNCTION(BlueprintCallable, Category = GameState)
	int32 GetNumberOfPlayers() const;
	UFUNCTION(BlueprintCallable, Category = GameState)
	int32 GetNumberOfAlivePlayers() const;

	UFUNCTION(BlueprintCallable, Category = GameState)
	uint8 GetGameDifficulty() const { return GameDifficulty; }
	
	UFUNCTION(BlueprintCallable, Category = GameState)
	int32 GetNumberOfPlayersForScaling() const { return GetNumberOfPlayers(); }
	UFUNCTION(BlueprintCallable, Category = GameState)
	int32 GetNumberOfAlivePlayersForScaling() const { return GetNumberOfAlivePlayers(); }
	UFUNCTION(BlueprintCallable, Category = GameState)
	uint8 GetGameDifficultyForScaling() const { return GetGameDifficulty(); }

	USpawnCharacterSystem* GetSpawnCharacterSystem() const { return SpawnCharacterSystem; }

public:
	UPROPERTY(BlueprintAssignable, Category = Objective)
	FMatchStateChanged OnMatchStateChanged;

	UPROPERTY(BlueprintAssignable, Category = Objective)
	FPlayerArrayChangeSignature OnPlayerStateAdded;
	UPROPERTY(BlueprintAssignable, Category = Objective)
	FPlayerArrayChangeSignature OnPlayerStateRemoved;

	UPROPERTY(BlueprintAssignable, Category = Objective)
	FPlayerSpectatorChangedSignature OnPlayerSpectatorChanged;
	UPROPERTY(BlueprintAssignable, Category = Objective)
	FPlayerAliveChangedSignature OnPlayerAliveChanged;

	//Broadcasted when a player-owned playerstate becomes relevant (joining the game, joining from spectator, etc.)
	UPROPERTY(BlueprintAssignable, Category = Objective)
	FPlayerStateRelevancyChangeSignature OnPlayerStateBecomeRelevant;

	//Broadcasted when a player-owned playerstate becomes non-relevant (leaving the game, joining spectator, etc.)
	UPROPERTY(BlueprintAssignable, Category = Objective)
	FPlayerStateRelevancyChangeSignature OnPlayerStateBecomeNonRelevant;

protected:
	UFUNCTION()
	virtual void ApplyFriendlyFireDamageMultiplier(AActor* Actor, float& DamageAmount, FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser);
	UFUNCTION()
	virtual void ApplyFriendlyFireEffectPowerMultiplier(AActor* Actor, float& EffectPower, FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION()
	void OnRep_PlayerClassList();

	UFUNCTION()
	void BindToRelevantPlayerState(ACorePlayerState* PlayerState);
	UFUNCTION()
	void UnbindToRelevantPlayerState(ACorePlayerState* PlayerState);

	UFUNCTION()
	void OnPlayerSpectatorUpdate(ACorePlayerState* PlayerState, bool bIsSpectator);
	UFUNCTION()
	void OnPlayerAliveUpdate(ACorePlayerState* PlayerState, bool bAlive);

protected:
	UPROPERTY(EditDefaultsOnly, Category = GameState)
	TArray<TSoftClassPtr<UPlayerClassComponent>> DefaultPlayerClassList;
	
	UPROPERTY(EditDefaultsOnly, Category = GameState)
	float FriendlyFireDamageMultiplier = 0.01f;

	UPROPERTY(EditDefaultsOnly, Category = GameState)
	float FriendlyFireEffectPowerMultiplier = 0.f;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_PlayerClassList)
	TArray<TSubclassOf<UPlayerClassComponent>> PlayerClassList;
	UPROPERTY(Transient, Replicated)
	uint8 GameDifficulty = 0;
	
	//List of player states that are owned by players, are active, and not spectators.
	UPROPERTY(Transient)
	TArray<ACorePlayerState*> RelevantCorePlayerStateList;

	UPROPERTY(Transient)
	USpawnCharacterSystem* SpawnCharacterSystem = nullptr;

public:
	/** Returns the current CoreGameState or Null if it can't be retrieved */
	UFUNCTION(BlueprintPure, Category="Game", meta=(WorldContext="WorldContextObject"))
	static ACoreGameState* GetCoreGameState(const UObject* WorldContextObject);
};
