// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Objective/Objective.h"
#include "Gameplay/CoreDamageType.h"
#include "ObjectiveWave.generated.h"

class ACoreCharacter;
class UStatusComponent;
class IWaveSpawnLocationInterface;

UENUM(BlueprintType)
enum class EWaveStatus : uint8
{
	Inactive,
	InProgress,
	Intermission,
	Completed,
	MAX = 255
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWaveSizeChangedSignature, UObjectiveWave*, Objective, int32, NewTotalWaveSize);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWaveStatusChangedSignature, UObjectiveWave*, Objective, EWaveStatus, NewWaveStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FIntermissionTimeChangedSignature, UObjectiveWave*, Objective, float, IntermissionStartTime, float, IntermissionEndTime);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCurrentWaveChangedSignature, UObjectiveWave*, Objective, int32, CurrentWave);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTotalWaveCountChangedSignature, UObjectiveWave*, Objective, int32, TotalWaveCount);

/**
 * 
 */
UCLASS()
class NAUSEA_API UObjectiveWave : public UObjective
{
	GENERATED_UCLASS_BODY()

//~ Begin UObjective Interface
public:
	virtual void Initialize(UMissionComponent* MissionComponent) override;
	virtual void SetObjectiveState(EObjectiveState State) override;
	virtual FText GetObjecitveName() const override;
	virtual FString DescribeObjectiveToGameplayDebugger() const;
//~ End UObjective Interface
	
public:
	UFUNCTION(BlueprintPure, Category = Objective, meta = (WorldContext = "WorldContextObject", DeterminesOutputType = "ObjectiveClass"))
	static UObjectiveWave* CreateWaveObjective(UObject* WorldContextObject, TSubclassOf<UObjectiveWave> ObjectiveClass, const FString& InObjectiveID, TSubclassOf<UObjectiveWaveConfig> WaveConfigOverrideClass, TArray<TScriptInterface<IWaveSpawnLocationInterface>> SpawnLocationList);

	template<class T>
	static T* CreateWaveObjective(UObject* WorldContextObject, TSubclassOf<UObjectiveWave> ObjectiveClass, const FString& InObjectiveID, TSubclassOf<UObjectiveWaveConfig> WaveConfigOverrideClass, TArray<TScriptInterface<IWaveSpawnLocationInterface>> SpawnLocationList)
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UObjectiveWave>::Value, "'T' template parameter to CreateWaveObjective must be derived from UObjectiveWave");
		return Cast<T>(CreateWaveObjective(WorldContextObject, ObjectiveClass, InObjectiveID, WaveConfigOverrideClass, SpawnLocationList));
	}

	//Request a spawn of an array of character classes. Will callback HandleFailedSpawn or ProcessSpawn depending on result. Returns the number of characters we've requested to spawn.
	UFUNCTION()
	int32 RequestSpawn(TArray<TSubclassOf<ACoreCharacter>>& SpawnRequest, UObjectiveWaveConfig* WaveConfig);
	//Called when a spawn fails with as much context as possible of the failed spawn.
	UFUNCTION()
	void HandleFailedSpawn(TSubclassOf<ACoreCharacter> CharacterClass, const FTransform& SpawnTransform, UObjectiveWaveConfig* WaveConfig);
	//Called when a spawn succeeds with as much context as possible. CharacterClass might seem redundant but we'll send it since we have it for failure state and it's possible we decided to override the spawned class elsewhere.
	UFUNCTION()
	void ProcessSpawn(ACoreCharacter* Character, TSubclassOf<ACoreCharacter> CharacterClass, const FTransform& SpawnTransform, UObjectiveWaveConfig* WaveConfig);

	UFUNCTION()
	TScriptInterface<IWaveSpawnLocationInterface> GetSpawnTransform(TSubclassOf<ACoreCharacter> SpawnRequest, int32& RequestID, FTransform& SpawnTransform) const;

	//Fallback used by GetSpawnTransform()
	UFUNCTION()
	TScriptInterface<IWaveSpawnLocationInterface> GetRandomSpawnTransform(TSubclassOf<ACoreCharacter> SpawnRequest, FTransform& SpawnTransform) const;

	UFUNCTION(BlueprintCallable, Category = Objective, BlueprintAuthorityOnly)
	void SetWaveStatus(EWaveStatus NewWaveStatus);
	UFUNCTION(BlueprintCallable, Category = Objective, BlueprintAuthorityOnly)
	void SetIntermissionTime(const FVector2D& NewIntermissionTime);
	UFUNCTION(BlueprintCallable, Category = Objective)
	EWaveStatus GetWaveStatus() const { return WaveStatus; }
	UFUNCTION(BlueprintCallable, Category = Objective)
	void GetIntermissionTime(float& StartTime, float& EndTime) const { StartTime = IntermissionTime.X; EndTime = IntermissionTime.Y; }

	UFUNCTION(BlueprintCallable, Category = Objective, BlueprintAuthorityOnly)
	void SetTotalWaveSizeCount(int32 NewTotalWaveSizeCount);
	UFUNCTION(BlueprintCallable, Category = Objective)
	int32 GetTotalWaveSizeCount() const { return TotalWaveSizeCount; }
	UFUNCTION(BlueprintCallable, Category = Objective)
	int32 GetRemainingWaveCount() const { return FMath::RoundToInt(float(TotalWaveSizeCount) * (1.f - GetObjecitveProgress())); }

	//Returns a formatted version of the objective name for the current wave (if the text supports it).
	UFUNCTION(BlueprintCallable, Category = Objective)
	FText GetFormattedWaveObjecitveName() const;
	
	UFUNCTION(BlueprintCallable, Category = Objective)
	int32 GetCurrentWave() const { return CurrentWave; }
	UFUNCTION(BlueprintCallable, Category = Objective)
	int32 GetTotalWaveCount() const { return TotalWaveCount; }

	UFUNCTION(BlueprintCallable, Category = Objective, BlueprintAuthorityOnly)
	void SetCurrentWave(int32 NewCurrentWave);
	UFUNCTION(BlueprintCallable, Category = Objective, BlueprintAuthorityOnly)
	void SetTotalWaveCount(int32 NewTotalWaveCount);

	virtual FString GetWaveStatusString() const;

public:
	UPROPERTY(BlueprintAssignable, Category = Objective)
	FWaveSizeChangedSignature OnObjectiveWaveSizeChanged;

	UPROPERTY(BlueprintAssignable, Category = Objective)
	FWaveStatusChangedSignature OnObjectiveWaveStatusChanged;

	UPROPERTY(BlueprintAssignable, Category = Objective)
	FIntermissionTimeChangedSignature OnObjectiveWaveIntermissionTimeChanged;

	UPROPERTY(BlueprintAssignable, Category = Objective)
	FCurrentWaveChangedSignature OnObjectiveCurrentWaveCountChanged;
	UPROPERTY(BlueprintAssignable, Category = Objective)
	FTotalWaveCountChangedSignature OnObjectiveTotalWaveCountChanged;

protected:
	UFUNCTION()
	void BindToWaveConfig();
	UFUNCTION()
	void UnbindToWaveConfig();

	UFUNCTION()
	void WaveConfigProgressUpdate(UObjectiveWaveConfig* Wave, float Progress);
	UFUNCTION()
	void WaveConfigStatusUpdate(UObjectiveWaveConfig* Wave, EWaveStatus Status);
	UFUNCTION()
	void WaveConfigCompleted(UObjectiveWaveConfig* Wave);
	UFUNCTION()
	void WaveConfigCurrentWaveUpdate(UObjectiveWaveConfig* Wave, int32 NewCurrentWave);
	UFUNCTION()
	void WaveConfigTotalWaveCountUpdate(UObjectiveWaveConfig* Wave, int32 NewTotalWaveCount);

	UFUNCTION()
	void OnRep_TotalWaveSizeCount();
	UFUNCTION()
	void OnRep_WaveStatus();
	UFUNCTION()
	void OnRep_IntermissionTime();

	UFUNCTION()
	void OnRep_TotalWaveCount();
	UFUNCTION()
	void OnRep_CurrentWave();

protected:
	UPROPERTY(Replicated, EditDefaultsOnly, Category = Objective)
	TSubclassOf<UObjectiveWaveConfig> WaveConfigClass = nullptr;
	UPROPERTY()
	UObjectiveWaveConfig* WaveConfigInstance = nullptr;
	
	//We don't replicate remaining spawn count and instead infer it through total size and progress.
	UPROPERTY(ReplicatedUsing = OnRep_TotalWaveSizeCount)
	int32 TotalWaveSizeCount = -1;

	//The wave config's current "status". Used to provide client with more info about what the wave config is doing.
	UPROPERTY(ReplicatedUsing = OnRep_WaveStatus)
	EWaveStatus WaveStatus = EWaveStatus::Inactive;

	//Refers to the start and end time of the current intermission. X is start time, Y is end time.
	UPROPERTY(ReplicatedUsing = OnRep_IntermissionTime)
	FVector2D IntermissionTime = FVector2D(-1.f);

	//The total number of waves this objective has. Can be -1 if not applicable.
	UPROPERTY(ReplicatedUsing = OnRep_TotalWaveCount)
	int32 TotalWaveCount = -1;
	//The current wave number this objective is on. Can be -1 if not applicable.
	UPROPERTY(ReplicatedUsing = OnRep_CurrentWave)
	int32 CurrentWave = -1;

	UPROPERTY()
	TArray<TScriptInterface<IWaveSpawnLocationInterface>> ObjectiveSpawnLocationList;

	//Score-sorted version of ObjectiveSpawnLocationList. const functions will mutate this to remove entries they can't use.
	UPROPERTY()
	mutable TArray<TScriptInterface<IWaveSpawnLocationInterface>> ScoredObjectiveSpawnLocationList;

	//The spawn location object currently being used. We cache this so that spawns can maintain coherence during recursion.
	UPROPERTY()
	mutable TScriptInterface<IWaveSpawnLocationInterface> UsedObjectiveSpawnLocation;
};

UENUM(BlueprintType)
enum class EEndWaveCharacterHandlingMethod : uint8
{
	Kill,
	Destroy,
	Ignore,
	MAX = 255
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWaveProgressUpdateSignature, UObjectiveWaveConfig*, Wave, float, Progress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWaveStatusUpdateSignature, UObjectiveWaveConfig*, Wave, EWaveStatus, Status);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWaveConfigCompletedSignature, UObjectiveWaveConfig*, Wave);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWaveConfigCurrentWaveChangedSignature, UObjectiveWaveConfig*, Wave, int32, CurrentWave);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWaveConfigTotalWaveCountChangedSignature, UObjectiveWaveConfig*, Wave, int32, TotalWaveCountChanged);

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, AutoExpandCategories = (Default))
class NAUSEA_API UObjectiveWaveConfig : public UObject
{
	GENERATED_UCLASS_BODY()

//~ Begin UObject Interface
public:
	virtual void PostInitProperties() override;
//~ End UObject Interface

public:
	virtual void Initialize();

	UFUNCTION()
	virtual void SetActive(bool bNewIsActive);

	//NOTE: Everything is auth-only for BP because wave config objects do not run on remote (and we do not want to confuse any future user about that).

	//Returns whether or not this wave is an active wave on an UObjectiveWave. The wave's status (in progress, intermission, etc.) is pushed to the owning objective and can be checked there.
	UFUNCTION(BlueprintCallable, Category = WaveConfig, BlueprintAuthorityOnly)
	bool IsActive() const { return bIsActive; }

	UFUNCTION(BlueprintCallable, Category = WaveConfig, BlueprintAuthorityOnly)
	UObjectiveWave* GetOwningObjective() const { return OwningObjective; }

	//Returns the time it will take to spawn again if a next spawn time is requested this frame.
	UFUNCTION(BlueprintCallable, Category = WaveConfig, BlueprintAuthorityOnly)
	virtual float GetTimeBetweenSpawns() const { return 1.f; }

	//Returns the time until the next spawn will occur. 
	UFUNCTION(BlueprintCallable, Category = WaveConfig, BlueprintAuthorityOnly)
	virtual float GetNextSpawnTime() const { return -1.f; }

	//Returns how many characters are alive due to this spawn wave.
	UFUNCTION(BlueprintCallable, Category = WaveConfig, BlueprintAuthorityOnly)
	virtual int32 GetNumberCurrentlySpawned() const { return CurrentlySpawnedCharacters.Num(); }

	//Returns how many spawns are remaining in this wave (currently spawned or not).
	UFUNCTION(BlueprintCallable, Category = WaveConfig, BlueprintAuthorityOnly)
	virtual int32 GetRemainingSpawnCount() const { return -1; }

	//Returns how many spawns this wave expects to spawn in total.
	UFUNCTION(BlueprintCallable, Category = WaveConfig, BlueprintAuthorityOnly)
	virtual int32 GetTotalSpawnCount() const { return -1; }

	//Returns how far along this wave is along (usually the result of GetRemainingSpawnCount() divided by GetTotalSpawnCount()).
	UFUNCTION(BlueprintCallable, Category = WaveConfig, BlueprintAuthorityOnly)
	virtual float GetWaveProgress() const { return -1.f; }

	UFUNCTION(BlueprintCallable, Category = WaveConfig, BlueprintAuthorityOnly)
	const FText& GetObjectiveNameOverride() const { return ObjectiveNameOverride; }

	//Returns how many spawns this wave expects to spawn in total.
	UFUNCTION(BlueprintCallable, Category = WaveConfig, BlueprintAuthorityOnly)
	virtual int32 GetCurrentWave() const { return -1; }

	//Returns how far along this wave is along (usually the result of GetRemainingSpawnCount() divided by GetTotalSpawnCount()).
	UFUNCTION(BlueprintCallable, Category = WaveConfig, BlueprintAuthorityOnly)
	virtual int32 GetTotalWaveCount() const { return -1.f; }

	//Returns how long this wave has been running for.
	UFUNCTION(BlueprintCallable, Category = WaveConfig, BlueprintAuthorityOnly)
	virtual float GetElapsedWaveTime() const { return GetWorld()->GetTimeSeconds() - WaveStartTime; }

	virtual FString DescribeWaveConfigToGameplayDebugger() const;

	UFUNCTION()
	virtual void ProcessSpawnedCharacter(ACoreCharacter* SpawnedCharacter);
	UFUNCTION()
	virtual void FailedSpawnCharacter(TSubclassOf<ACoreCharacter> CharacterClass);
	
public:
	UPROPERTY(BlueprintAssignable, Category = WaveConfig)
	FWaveProgressUpdateSignature OnWaveProgressUpdate;
	UPROPERTY(BlueprintAssignable, Category = WaveConfig)
	FWaveStatusUpdateSignature OnWaveStatusUpdate;
	UPROPERTY(BlueprintAssignable, Category = WaveConfig)
	FWaveConfigCompletedSignature OnWaveCompleted;

	UPROPERTY(BlueprintAssignable, Category = WaveConfig)
	FWaveConfigCurrentWaveChangedSignature OnWaveConfigCurrentWaveChanged;
	UPROPERTY(BlueprintAssignable, Category = WaveConfig)
	FWaveConfigTotalWaveCountChangedSignature OnWaveConfigTotalWaveCountChanged;

protected:
	UFUNCTION()
	virtual void StartSpawning();
	UFUNCTION()
	virtual void StopSpawning();
	UFUNCTION()
	virtual void SpawningCompleted();

	UFUNCTION()
	void SetWaveStatus(EWaveStatus NewWaveStatus);

	virtual void BindToSpawnedCharacter(ACoreCharacter* SpawnedCharacter);
	virtual void UnbindToSpawnedCharacter(ACoreCharacter* SpawnedCharacter);

	UFUNCTION()
	virtual void SpawnedCharacterDestroyed(AActor* Actor, EEndPlayReason::Type EndPlayReason);
	UFUNCTION()
	virtual void SpawnedCharacterDied(UStatusComponent* Component, float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

protected:
	UPROPERTY()
	bool bIsActive = false;
	UPROPERTY()
	float WaveStartTime = -1.f;

	UPROPERTY(EditDefaultsOnly, Category = WaveConfig)
	FText ObjectiveNameOverride = FText();

	UPROPERTY(EditDefaultsOnly, Category = WaveConfig)
	EEndWaveCharacterHandlingMethod EndWaveCharacterHandlingMethod = EEndWaveCharacterHandlingMethod::Kill;

private:
	UPROPERTY()
	TArray<TWeakObjectPtr<ACoreCharacter>> CurrentlySpawnedCharacters;

	UPROPERTY()
	UObjectiveWave* OwningObjective = nullptr;

	UPROPERTY()
	EWaveStatus WaveStatus = EWaveStatus::Inactive;
};

UCLASS()
class NAUSEA_API UWaveEndKillDamageType : public UCoreDamageType
{
	GENERATED_UCLASS_BODY()
};