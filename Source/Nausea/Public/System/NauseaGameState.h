// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "System/CoreGameState.h"
#include "NauseaGameState.generated.h"

class ACoreGameMode;
class ANauseaGameMode;
class ACorePlayerState;
class ANauseaPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerReadyChangedSignature, ANauseaPlayerState*, PlayerState, bool, bReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FForceStartCountdownStateChanged, ANauseaGameState*, GameState, bool, bStarted);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMissionChangedSignature, UMissionComponent*, Mission, UMissionComponent*, PreviousMission);

/**
 * 
 */
UCLASS()
class NAUSEA_API ANauseaGameState : public ACoreGameState
{
	GENERATED_UCLASS_BODY()

//~ Begin AGameStateBase Interface
public:
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
//~ End AGameStateBase Interface

//~ Begin ACoreGameState Interface
public:
	virtual void InitializeGameState(ACoreGameMode* CoreGameMode) override;
//~ End ACoreGameState Interface

public:
	virtual void InitializeMissions();

	UFUNCTION(BlueprintPure, Category = GameState)
	UMissionComponent* GetCurrentMission() const { return CurrentMission; }
	UFUNCTION(BlueprintPure, Category = GameState)
	UMissionComponent* GetInitialMission() const { return InitialMission; }

	UFUNCTION()
	void SetCurrentMission(UMissionComponent* InMission, bool bAutoActivate = true);

	//Called internally via ANauseaGameMode::OnPlayerReadyUp when the timer is started or stopped. Is only for clients to be notified of the countdown state changing.
	UFUNCTION()
	void SetForceStartCountdownTime(float InCountdownDuration);
	
	UFUNCTION(BlueprintPure, Category = GameState)
	float GetCountdownStartTime() const { return ForceStartCountdownTime.X; }
	UFUNCTION(BlueprintPure, Category = GameState)
	float GetForceStartCountdownTime() const { return ForceStartCountdownTime.Y; }
	UFUNCTION(BlueprintPure, Category = GameState)
	float GetCountdownPercentRemaining() const;

protected:
	UFUNCTION()
	void OnRep_CurrentMission(UMissionComponent* PreviousMission);

	UFUNCTION()
	void MissionCompleted(UMissionComponent* Mission);
	UFUNCTION()
	void MissionFailed(UMissionComponent* Mission);

	UFUNCTION()
	void OnPlayerReadyUpUpdate(ANauseaPlayerState* PlayerState, bool bReady);

	UFUNCTION()
	void OnRep_ForceStartCountdownTime();

public:
	UPROPERTY(BlueprintAssignable, Category = Objective)
	FPlayerReadyChangedSignature OnPlayerReadyChanged;

	UPROPERTY(BlueprintAssignable, Category = Objective)
	FMissionChangedSignature OnMissionChanged;
	
	UPROPERTY(BlueprintAssignable, Category = Objective)
	FForceStartCountdownStateChanged OnForceStartCountdownStateChanged;

protected:
	UPROPERTY(Transient, ReplicatedUsing = OnRep_CurrentMission)
	UMissionComponent* CurrentMission = nullptr;
	UPROPERTY(Transient, Replicated)
	UMissionComponent* InitialMission = nullptr;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_ForceStartCountdownTime)
	FVector2D ForceStartCountdownTime = FVector2D(-1.f);

public:
	/** Returns the current NauseaGameState or Null if it can't be retrieved */
	UFUNCTION(BlueprintPure, Category="Game", meta=(WorldContext="WorldContextObject"))
	static ANauseaGameState* GetNauseaGameState(const UObject* WorldContextObject);
};