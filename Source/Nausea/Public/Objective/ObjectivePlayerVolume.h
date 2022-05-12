// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Objective/Objective.h"
#include "ObjectivePlayerVolume.generated.h"

class AActor;
class ACoreCharacter;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FObjectivePlayerStatusChangedSignature, UObjectivePlayerVolume*, Objective, ACoreCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FObjectivePlayerListChangedSignature, UObjectivePlayerVolume*, Objective);

/**
 * 
 */
UCLASS()
class NAUSEA_API UObjectivePlayerVolume : public UObjective
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UObjective Interface
public:
	virtual void SetObjectiveState(EObjectiveState State) override;
	virtual FString DescribeObjectiveToGameplayDebugger() const override;
//~ End UObjective Interface

public:
	UFUNCTION(BlueprintPure, Category = Objective, meta = (WorldContext = "WorldContextObject", DeterminesOutputType = "ObjectiveClass"))
	static UObjectivePlayerVolume* CreatePlayerVolumeObjective(UObject* WorldContextObject, TSubclassOf<UObjectivePlayerVolume> ObjectiveClass, const FString& InObjectiveID, TArray<AActor*> InOverlapActors);

	template<class T>
	static T* CreatePlayerVolumeObjective(UObject* WorldContextObject, TSubclassOf<UObjectivePlayerVolume> ObjectiveClass, const FString& InObjectiveID, TArray<AActor*> InOverlapActors)
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UObjectivePlayerVolume>::Value, "'T' template parameter to CreateWaitForPlayersObjective must be derived from UObjectivePlayerVolume");
		return Cast<T>(CreatePlayerVolumeObjective(WorldContextObject, ObjectiveClass, InObjectiveID, InOverlapActors));
	}

	//Not trivial due to actor iterator so not Blueprint Pure.
	UFUNCTION(BlueprintCallable, Category = Objective)
	float GetPercentCharactersInObjective() const;

	UFUNCTION(BlueprintCallable, Category = Objective)
	int32 GetNumberOfConsideredCharacters() const;
	UFUNCTION(BlueprintCallable, Category = Objective)
	int32 GetNumberOfOverlappingCharacters() const;
	
	UFUNCTION(BlueprintCallable, Category = Objective)
	TArray<ACoreCharacter*> GetOverlappingCharacters() const;

protected:
	UFUNCTION()
	void BindObjectiveEvents();
	UFUNCTION()
	void UnBindObjectiveEvents();
	UFUNCTION()
	void GenerateInitialOverlaps();

	UFUNCTION()
	void OnOverlapBegin(AActor* OverlapActor, AActor* OtherActor);
	UFUNCTION()
	void OnOverlapEnd(AActor* OverlapActor, AActor* OtherActor);

	UFUNCTION()
	void AddCharacter(ACoreCharacter* Character);
	UFUNCTION()
	void RemoveCharacter(ACoreCharacter* Character);

	UFUNCTION()
	void CharacterDestroyed(AActor* Character);

	//Called when a player within the volume is killed.
	UFUNCTION()
	void CharacterKilled(UStatusComponent* Component, float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	//Called whenever any character is killed.
	UFUNCTION()
	void NotifyKilled(AController* Killer, AController* Killed, ACoreCharacter* KilledCharacter, const struct FDamageEvent& DamageEvent);

	UFUNCTION()
	void OnRep_OverlappingCharacters(TArray<TWeakObjectPtr<ACoreCharacter>> PreviousOverlappingCharacters);

	UFUNCTION(BlueprintImplementableEvent, Category = Objective, meta = (DisplayName = "On Player Entered Objective", ScriptName = "OnPlayerEnteredObjective"))
	void K2_OnPlayerEntered(ACoreCharacter* Character);
	UFUNCTION(BlueprintImplementableEvent, Category = Objective, meta = (DisplayName = "On Player Exited Objective", ScriptName = "OnPlayerExitedObjective"))
	void K2_OnPlayerExited(ACoreCharacter* Character);
	UFUNCTION(BlueprintImplementableEvent, Category = Objective, meta = (DisplayName = "On Player List Changed", ScriptName = "OnPlayerListChanged"))
	void K2_OnPlayerListChanged();

	UFUNCTION(BlueprintImplementableEvent, Category = Objective, meta = (DisplayName = "On Player Killed", ScriptName = "OnPlayerKilled"))
	void K2_OnPlayerKilled(AController* Killer, AController* Killed, ACoreCharacter* KilledCharacter, const struct FDamageEvent& DamageEvent);

public:
	UPROPERTY(BlueprintAssignable, Category = Objective)
	FObjectivePlayerStatusChangedSignature OnPlayerEnteredObjective;
	UPROPERTY(BlueprintAssignable, Category = Objective)
	FObjectivePlayerStatusChangedSignature OnPlayerExitedObjective;
	UPROPERTY(BlueprintAssignable, Category = Objective)
	FObjectivePlayerListChangedSignature OnPlayerListChanged;
	
protected:
	UPROPERTY()
	TArray<AActor*> OverlapActors;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingCharacters)
	TArray<TWeakObjectPtr<ACoreCharacter>> OverlappingCharacters;

	UPROPERTY(EditDefaultsOnly, Category = Objective)
	bool bPercentOfAliveCharacters = true;
};
