// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTags/Classes/GameplayTagContainer.h"
#include "System/CoreGameModeSettings.h"
#include "System/SpawnCharacterSystem.h"
#include "DungeonGameModeSettings.generated.h"

class ADungeonCharacter;
class ADungeonGameMode;

UCLASS()
class NAUSEA_API UDungeonGameModeSettings : public UCoreGameModeSettings
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = DungeonGameMode)
	int64 CalculateStartingGameHealth(ADungeonGameMode* GameMode) const;

	UFUNCTION(BlueprintCallable, Category = DungeonGameMode)
	int32 CalculateStartingTrapCoinAmount(ADungeonGameMode* GameMode) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = DungeonGameMode)
	int64 StartingGameHealth = 100;

	UPROPERTY(EditDefaultsOnly, Category = DungeonGameMode)
	int32 StartingTrapCoinAmount = 1000;

	UPROPERTY(EditDefaultsOnly, Category = DungeonGameMode)
	int64 FinalWaveOverride = -1;
};


UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EWaveModifierType : uint8
{
	None = 0,
	Override = 1 << 1,
	Additive = 1 << 2
};
ENUM_CLASS_FLAGS(EWaveModifierType);

class UCurveFloat;

USTRUCT(BlueprintType)
struct FWaveModifierEntry
{
	GENERATED_USTRUCT_BODY()

public:
	FWaveModifierEntry() {}
	FWaveModifierEntry(UWaveModifier* InModifier, UWaveConfiguration* InConfiguration)
		: Modifier(InModifier), Configuration(InConfiguration) {}

	FORCEINLINE UWaveModifier* GetModifier() const { return Modifier; }
	FORCEINLINE UWaveConfiguration* GetConfiguration() const { return Configuration; }

protected:
	UPROPERTY(EditAnywhere, Category = Entry, Instanced)
	UWaveModifier* Modifier = nullptr;
	UPROPERTY(EditAnywhere, Category = Entry, Instanced)
	UWaveConfiguration* Configuration = nullptr;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWaveCompletedSignature, UDungeonWaveSetup*, WaveSetup, int64, WaveNumber);

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, AutoExpandCategories = (WaveSetup))
class NAUSEA_API UDungeonWaveSetup : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = WaveSetup)
	TArray<UWaveConfiguration*> GetWaveConfiguration(int64 WaveNumber) const;

	UFUNCTION(BlueprintCallable, Category = WaveSetup)
	int64 InitializeWaveSetup(int64 WaveNumber);
	UFUNCTION(BlueprintCallable, Category = WaveSetup)
	bool StartWave(int64 WaveNumber);

	UFUNCTION(BlueprintCallable, Category = WaveSetup)
	void GetCharacterClassListForWave(int64 WaveNumber, TArray<TSubclassOf<ADungeonCharacter>>& DungeonCharacterClassList) const;

	UFUNCTION()
	void OnWaveCharacterListEmpty();

	UFUNCTION(BlueprintCallable, Category = WaveSetup)
	bool IsWaveAutoStart(const TArray<UWaveConfiguration*>& Configuration) const;
	UFUNCTION(BlueprintCallable, Category = WaveSetup)
	int32 GetWaveAutoStartTime(const TArray<UWaveConfiguration*>& Configuration) const;

	UFUNCTION(BlueprintCallable, Category = WaveSetup)
	bool IsFinalWave(int64 WaveNumber) const { return WaveNumber >= LastWaveNumber; }

	UFUNCTION(BlueprintCallable, Category = WaveSetup)
	UCurveFloat* GetDefaultWaveSizeScaling() const { return DefaultWaveSizeScaling; }
	UFUNCTION(BlueprintCallable, Category = WaveSetup)
	UCurveFloat* GetDefaultWaveRateScaling() const { return DefaultWaveRateScaling; }

	UFUNCTION(BlueprintCallable, Category = WaveSetup)
	int64 GetExpectedSpawnCount() const { return TotalExpectedSpawnCount; }

	UFUNCTION(BlueprintCallable, Category = WaveSetup, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext, AdvancedDisplay = 3))
	static UDungeonWaveSetup* CreateWaveSetup(UObject* WorldContextObject, TSubclassOf<UDungeonWaveSetup> SetupClass, const TArray<FWaveModifierEntry>& AdditionalModifiers, bool bClearSetupClassModifiers = false,
		UWaveConfiguration* DefaultWaveConfigurationOverride = nullptr, UCurveFloat* DefaultWaveSizeScalingOverride = nullptr, UCurveFloat* DefaultWaveRateScalingOverride = nullptr, int64 LastWaveNumberOverride = -1);

	//Helper pure function. Sole purpose is to make a modifier entry from a modifier and a configuration.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = WaveSetup)
	static FWaveModifierEntry CreateModifierEntry(UWaveModifier* Modifier, UWaveConfiguration* Configuration);

public:
	UPROPERTY(BlueprintAssignable, Category = WaveSetup)
	FOnWaveCompletedSignature OnWaveCompleted;

protected:
	//Last wave. If 0, we assume this endless.
	UPROPERTY(EditAnywhere, Category = WaveSetup, meta = (ClampMin = "0"))
	int64 LastWaveNumber = 10;

	UPROPERTY(EditAnywhere, Category = WaveSetup)
	UCurveFloat* DefaultWaveSizeScaling = nullptr;
	UPROPERTY(EditAnywhere, Category = WaveSetup)
	UCurveFloat* DefaultWaveRateScaling = nullptr;

	UPROPERTY(EditAnywhere, Category = WaveSetup, Instanced)
	UWaveConfiguration* DefaultWaveConfiguration = nullptr;

	UPROPERTY(EditAnywhere, Category = WaveSetup)
	TArray<FWaveModifierEntry> ModifierList;

	UPROPERTY(Transient)
	int64 InitializedWave = -1;
	UPROPERTY(Transient)
	int64 TotalExpectedSpawnCount = -1;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, AutoExpandCategories = (Configuration))
class NAUSEA_API UWaveConfiguration : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	//Initializes this configuration for the given wave number. Returns the total number of spawns.
	UFUNCTION()
	int32 InitializeForWave(int64 WaveNumber, UDungeonWaveSetup* Setup = nullptr);
	UFUNCTION()
	void CleanUp();
	UFUNCTION()
	bool IsInitialized() const { return InitializedWaveNumber != INDEX_NONE; }

	UFUNCTION()
	void StartSpawning();

	UFUNCTION()
	bool IsDoneSpawning() const;
	UFUNCTION()
	int32 NumberRemainingToSpawn() const;
	//Is the spawn timer to fire another spawn active.
	UFUNCTION()
	bool IsSpawnIntervalTimerActive() const;

	UFUNCTION(BlueprintCallable, Category = Configuration)
	int32 GetSpawnCount(int64 WaveNumber, UDungeonWaveSetup* Setup = nullptr) const;
	UFUNCTION(BlueprintCallable, Category = Configuration)
	int32 GetTimeOffset(int64 WaveNumber, UDungeonWaveSetup* Setup = nullptr) const;
	UFUNCTION(BlueprintCallable, Category = Configuration)
	int32 GetTimeInterval(int64 WaveNumber, UDungeonWaveSetup* Setup = nullptr) const;
	UFUNCTION(BlueprintCallable, Category = Configuration)
	int32 GetSpawnBatchAmount(int64 WaveNumber, UDungeonWaveSetup* Setup = nullptr) const;

	void AppendCharacterClassListForWave(int64 WaveNumber, TArray<TSubclassOf<ADungeonCharacter>>& DungeonCharacterClassList) const;

	UFUNCTION(BlueprintCallable, Category = Configuration)
	bool IsAutoStartWave() const { return bIsAutoStartWave; }
	UFUNCTION(BlueprintCallable, Category = Configuration)
	int32 GetAutoStartTime() const { return AutoStartTime; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = WaveSetup, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext, AdvancedDisplay = 2))
	static UWaveConfiguration* CreateWaveConfiguration(UObject* WorldContextObject, TSubclassOf<UWaveConfiguration> WaveClass, FGameplayTagContainer AdditionalSpawnerTags,
		bool bClearWaveClassSpawnTags = false, int32 BaseSpawnCountOverride = -1, int32 SpawnTimeOffsetOverride = -1, int32 SpawnIntervalTimeOverride = -1, int32 SpawnBatchAmountOverride = -1);

protected:
	//Stops spawning on this object. Cancels latent actions, timers, etc.
	UFUNCTION()
	void StopSpawning();

	UFUNCTION()
	void SetNextSpawnInterval();

	UFUNCTION()
	int32 PerformSpawn();
	UFUNCTION()
	void OnSpawnRequestResult(const FSpawnRequest& Request, ACoreCharacter* Character);
	UFUNCTION()
	void OnSpawnFailed(const FSpawnRequest& Request);

	void RefundFailedSpawn(TSubclassOf<ADungeonCharacter> CharacterClass);

	TArray<TSubclassOf<ADungeonCharacter>> GetNextSpawnList();
	
	TArray<ISpawnLocationInterface*> GetSpawnLocationList();
	
protected:
	UPROPERTY(EditAnywhere, Category = Configuration, meta = (ClampMin = "0"))
	int32 BaseSpawnCount = 12;
	UPROPERTY(EditAnywhere, Category = Scaling)
	UCurveFloat* WaveSizeScalingCurve = nullptr;
	//If true, will ignore the world settings default size scaling curve.
	UPROPERTY(EditAnywhere, Category = Scaling)
	bool bIgnoreSettingsWaveSizeScalingCurve = false;

	UPROPERTY(EditAnywhere, Category = Configuration, Instanced)
	TArray<UWaveSpawnGroup*> SpawnWaveGroups;

	//How long to delay before firing the first spawn of this configuration.
	UPROPERTY(EditAnywhere, Category = Configuration, meta = (ClampMin = "0"))
	int32 SpawnTimeOffset = 0;
	UPROPERTY(EditAnywhere, Category = Scaling)
	UCurveFloat* SpawnTimeOffsetWaveCurve = nullptr;
	//If true, initial time offset will ignore the world settings default spawn rate scaling curve.
	UPROPERTY(EditAnywhere, Category = Scaling)
	bool bIgnoreSettingsWaveSpawnRateCurveForTimeOffset = false;

	//How long to delay between firing of the listed spawns for this configuration.
	UPROPERTY(EditAnywhere, Category = Configuration, meta = (ClampMin = "1"))
	int32 SpawnIntervalTime = 1;
	UPROPERTY(EditAnywhere, Category = Scaling)
	UCurveFloat* SpawnIntervalTimeWaveCurve = nullptr;
	//If true, spawn interval will ignore the world settings default spawn rate scaling curve.
	UPROPERTY(EditAnywhere, Category = Scaling)
	bool bIgnoreSettingsWaveSpawnRateCurveForTimeInterval = false;

	//Number of characters to spawn per fire. Is limited by spawns remaining on this config at runtime.
	UPROPERTY(EditAnywhere, Category = Configuration, meta = (ClampMin = "0"))
	int32 SpawnBatchAmount = 4;
	UPROPERTY(EditAnywhere, Category = Scaling)
	UCurveFloat* SpawnBatchAmountWaveCurve = nullptr;
	//If true, spawn batch amount will ignore the world settings default spawn size scaling curve.
	UPROPERTY(EditAnywhere, Category = Scaling)
	bool bIgnoreSettingsWaveSpawnSizeCurveForSpawnBatchAmount = false;
	
	//Determines if this wave will autostart.
	UPROPERTY(EditAnywhere, Category = Configuration)
	bool bIsAutoStartWave = false;
	UPROPERTY(EditAnywhere, Category = Configuration, meta = (EditCondition="bIsAutoStartWave", ClampMin = "0"))
	int32 AutoStartTime = 10;

	UPROPERTY(EditAnywhere, Category = Configuration)
	FGameplayTagContainer SpawnerTags;

//==========
//TRANSIENTS
// 
	//Used to check if this configuration is in use (and what wave it's in use for).
	UPROPERTY(Transient)
	int64 InitializedWaveNumber = -1;
	//Calculated total value of spawns this configuration will be responsible for during this wave.
	UPROPERTY(Transient)
	int32 TotalSpawnCount = -1;
	//Number of characters spawned by this configuration during this wave.
	UPROPERTY(Transient)
	int32 NumberSpawned = -1;
	//Calculated fire time offset for this config for the specified wave.
	UPROPERTY(Transient)
	int32 CurrentSpawnTimeOffset = -1;
	//Calculated time between fires for this config for the specified wave.
	UPROPERTY(Transient)
	int32 CurrentSpawnInterval = -1;
	//Calculated time between fires for this config for the specified wave.
	UPROPERTY(Transient)
	int32 CurrentSpawnBatchAmount = -1;
	
	//How many spawns have been requested by this config (and have not been fufilled yet).
	UPROPERTY(Transient)
	int32 RequestedSpawnCount = 0;

	//Handle for the next spawn fire.
	UPROPERTY(Transient)
	FTimerHandle NextSpawnIntervalTimerHandle;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, AutoExpandCategories = (Modifier))
class NAUSEA_API UWaveModifier : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = Modifier)
	bool AppliesToWave(int64 WaveNumber) const;
	
	UFUNCTION(BlueprintCallable, Category = Modifier)
	uint8 GetModifierPriority() const { return Priority; }

	UFUNCTION(BlueprintCallable, Category = Modifier)
	bool IsOverrideWave() const { return ModifierType == EWaveModifierType::Override; }
	UFUNCTION(BlueprintCallable, Category = Modifier)
	bool IsAdditiveWave() const { return ModifierType == EWaveModifierType::Additive; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = WaveSetup, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext, AdvancedDisplay = 2))
	static UWaveModifier* CreateWaveModifier(UObject* WorldContextObject, TSubclassOf<UWaveModifier> ModifierClass,
		EWaveModifierType ModifierTypeOverride = EWaveModifierType::None, int32 StartingWaveNumberOverride = -1, int32 PriorityOverride = -1);

protected:
	UPROPERTY(EditAnywhere, Category = Modifier)
	uint8 Priority = 0;

	//Modifier will only apply to waves starting at this wave number (will override DesiredWaveNumber and DesiredWaveInterval).
	UPROPERTY(EditAnywhere, Category = Modifier, meta = (ClampMin = "0"))
	int64 StartingWaveNumber = 0;

	UPROPERTY(EditAnywhere, Category = Modifier, meta = (ClampMin = "0"))
	TArray<uint64> DesiredWaveNumber = TArray<uint64>();
	UPROPERTY(EditAnywhere, Category = Modifier, meta = (ClampMin = "0"))
	TArray<uint64> DesiredWaveInterval = TArray<uint64>();
	
	//Type of modifier this configuration applies. Override to ignore all configurations of lower priority, additive to add to base configurations of equal or lower priority.
	UPROPERTY(EditAnywhere, Category = Modifier, meta = (Bitmask, BitmaskEnum = EWaveModifierType))
	EWaveModifierType ModifierType = EWaveModifierType::Additive;
};

USTRUCT(BlueprintType, Blueprintable)
struct FWaveSpawnGroupEntry
{
	GENERATED_USTRUCT_BODY()

public:
	FWaveSpawnGroupEntry() {}

	void Initialize();

	bool HasRemainingSpawns() const { return RemainingToSpawn > 0; }
	TSubclassOf<ADungeonCharacter> GetNextSpawnClass();
	bool RefundSpawnClass(TSubclassOf<ADungeonCharacter> CharacterClass);

	TSubclassOf<ADungeonCharacter> GetClass() const;

protected:
	UPROPERTY(EditAnywhere, Category = SpawnGroup)
	TSoftClassPtr<ADungeonCharacter> Character = nullptr;
	UPROPERTY(EditAnywhere, Category = SpawnGroup)
	uint8 SpawnCount = 4;
	UPROPERTY(Transient)
	mutable TSubclassOf<ADungeonCharacter> LoadedCharacter = nullptr;
	UPROPERTY(Transient)
	uint8 RemainingToSpawn = 0;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, AutoExpandCategories = (SpawnGroup))
class NAUSEA_API UWaveSpawnGroup : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	void Initialize();

	void AppendCharacterClassListForWave(int64 WaveNumber, TArray<TSubclassOf<ADungeonCharacter>>& DungeonCharacterClassList) const;

	bool HasRemainingSpawns() const;
	TSubclassOf<ADungeonCharacter> GetNextSpawnClass();
	bool RefundSpawnClass(TSubclassOf<ADungeonCharacter> CharacterClass);

protected:
	UPROPERTY(EditAnywhere, Category = SpawnGroup)
	TArray<FWaveSpawnGroupEntry> SpawnList;
};