// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/PlayerClass/PlayerClassTypes.h"
#include "Player/PlayerOwnershipInterface.h"
#include "PlayerStatisticsTypes.generated.h"

class UPlayerClassComponent;
class UStatusEffectBase;

template<typename InKeyType, typename InValueType>
struct TMapPairStruct
{
	typedef InKeyType KeyType;
	typedef InValueType ValueType;
	TMapPairStruct() {}
	TMapPairStruct(KeyType InKey, ValueType InValue) { Key = InKey; Value = InValue; }
	
	friend FArchive& operator<<(FArchive& Ar, TMapPairStruct<KeyType, ValueType>& Entry) { Ar << Entry.Key; Ar << Entry.Value; return Ar; }

	FArchive& operator<<(FArchive& Ar) { Ar << Key; Ar << Value; return Ar; }

public:
	KeyType Key;
	ValueType Value;

public:
	FORCEINLINE static bool NetSerializeTemplateMap(FArchive& Ar, TMap<KeyType, ValueType>& SerializedMap)
	{
		const bool bIsLoading = Ar.IsLoading();
		TArray<TMapPairStruct<KeyType, ValueType>> DataArray;

		if (!bIsLoading)
		{
			for (const TPair<KeyType, ValueType>& Entry : SerializedMap)
			{
				DataArray.Add(TMapPairStruct<KeyType, ValueType>(Entry.Key, Entry.Value));
			}
		}

		Ar << DataArray;

		if (bIsLoading)
		{
			SerializedMap.Empty(SerializedMap.Num());
			for (const TMapPairStruct<KeyType, ValueType>& Entry : DataArray)
			{
				SerializedMap.Add(Entry.Key) = Entry.Value;
			}
		}

		return true;
	}
};

//----------------
//EXPERIENCE

USTRUCT()
struct NAUSEA_API FVariantExperienceMap
{
	GENERATED_USTRUCT_BODY()

public:
	const static uint64 InvalidExperienceValue;
	const static FVariantExperienceMap InvalidVariantExperienceMap;

	FVariantExperienceMap() {}

	FVariantExperienceMap(const FVariantExperienceMap& InSelectionMap)
	{
		VariantExperienceMap = InSelectionMap.VariantExperienceMap;
	}

	FVariantExperienceMap(const TMap<EPlayerClassVariant, uint64>& InMap)
	{
		VariantExperienceMap = InMap;
	}

	friend FArchive& operator<<(FArchive& Ar, FVariantExperienceMap& Entry);

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

public:
	uint64& operator[] (EPlayerClassVariant Variant) { return VariantExperienceMap[Variant]; }
	const uint64& operator[] (EPlayerClassVariant Variant) const { return VariantExperienceMap[Variant]; }

	const uint64& GetValue(EPlayerClassVariant Variant) const;

	uint64 AddValue(EPlayerClassVariant Variant, uint64 Delta);
	uint64 SetValue(EPlayerClassVariant Variant, uint64 InValue);

	FORCEINLINE bool Contains(EPlayerClassVariant Variant) const { return VariantExperienceMap.Contains(Variant); }
	FORCEINLINE int32 Remove(EPlayerClassVariant Variant) { return VariantExperienceMap.Remove(Variant); }
	FORCEINLINE uint64& FindOrAdd(EPlayerClassVariant Variant) { return VariantExperienceMap.FindOrAdd(Variant); }

	FORCEINLINE const TMap<EPlayerClassVariant, uint64>& Get() const { return VariantExperienceMap; }
	FORCEINLINE TMap<EPlayerClassVariant, uint64>& Get() { return VariantExperienceMap; }

protected:
	UPROPERTY()
	TMap<EPlayerClassVariant, uint64> VariantExperienceMap = TMap<EPlayerClassVariant, uint64>();
};
template<>
struct TStructOpsTypeTraits<FVariantExperienceMap> : public TStructOpsTypeTraitsBase2<FVariantExperienceMap>
{
	enum
	{
		WithNetSerializer = true
	};
};

USTRUCT(BlueprintType)
struct FExperienceStruct
{
	GENERATED_USTRUCT_BODY()

	FExperienceStruct() {}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

public:
	const static TMap<TSoftClassPtr<UPlayerClassComponent>, uint64> InvalidExperienceStruct;

	const FVariantExperienceMap& operator[] (TSoftClassPtr<UPlayerClassComponent> PlayerClass) const;

	bool IsValid(const FVariantExperienceMap& ExperienceValue) const; //Checks if a given reference to an uint64 is a valid one (not a reference to FExperienceStruct::InvalidExperienceValue.

	const uint64& GetValue(TSoftClassPtr<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant) const;

	uint64 AddValue(TSoftClassPtr<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, uint64 Delta);
	uint64 SetValue(TSoftClassPtr<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, uint64 InValue);

	FORCEINLINE const TMap<TSoftClassPtr<UPlayerClassComponent>, FVariantExperienceMap>& Get() const { return PlayerClassExperienceMap; }
	FORCEINLINE TMap<TSoftClassPtr<UPlayerClassComponent>, FVariantExperienceMap>& Get() { return PlayerClassExperienceMap; }

	bool ReplaceWithLarger(const FExperienceStruct& Other);

protected:
	UPROPERTY(NotReplicated)
	TMap<TSoftClassPtr<UPlayerClassComponent>, FVariantExperienceMap> PlayerClassExperienceMap = TMap<TSoftClassPtr<UPlayerClassComponent>, FVariantExperienceMap>();
};
template<>
struct TStructOpsTypeTraits<FExperienceStruct> : public TStructOpsTypeTraitsBase2<FExperienceStruct>
{
	enum
	{
		WithNetSerializer = true
	};
};

//----------------
//PLAYER STATS


UENUM(BlueprintType)
enum class EPlayerStatisticType : uint8
{
	Invalid,
	DamageDealt,
	DamageReceived,
	DamageHealed
};

USTRUCT(BlueprintType)
struct FPlayerStatisticsStruct
{
	GENERATED_USTRUCT_BODY()

	FPlayerStatisticsStruct() {}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	FORCEINLINE uint64 Get(EPlayerStatisticType StatisticType) const { return PlayerStatisticMap.Contains(StatisticType) ? PlayerStatisticMap[StatisticType] : 0; }
	FORCEINLINE uint64 Set(EPlayerStatisticType StatisticType, uint64 Value) { return PlayerStatisticMap.FindOrAdd(StatisticType) = Value; }
	FORCEINLINE uint64 Add(EPlayerStatisticType StatisticType, uint64 Delta) { return PlayerStatisticMap.FindOrAdd(StatisticType) += Delta; }

	FORCEINLINE const TMap<EPlayerStatisticType, uint64>& Get() const { return PlayerStatisticMap; }
	FORCEINLINE TMap<EPlayerStatisticType, uint64>& GetMutable() { return PlayerStatisticMap; }

	bool ReplaceWithLarger(const FPlayerStatisticsStruct& Other);

protected:
	UPROPERTY(NotReplicated)
	TMap<EPlayerStatisticType, uint64> PlayerStatisticMap = TMap<EPlayerStatisticType, uint64>();
};
template<>
struct TStructOpsTypeTraits<FPlayerStatisticsStruct> : public TStructOpsTypeTraitsBase2<FPlayerStatisticsStruct>
{
	enum
	{
		WithNetSerializer = true
	};
};

//----------------
//PLAYER SELECTION


class UInventory;

USTRUCT()
struct NAUSEA_API FInventorySelectionEntry
{
	GENERATED_USTRUCT_BODY()

public:
	typedef TArray<TSoftClassPtr<UInventory>> InventoryArrayType;

	FInventorySelectionEntry() {}

	FInventorySelectionEntry(const FInventorySelectionEntry& InWeaponSelection)
	{
		ConfigArray = InWeaponSelection.ConfigArray;
	}

	FInventorySelectionEntry(const InventoryArrayType& InArray)
	{
		ConfigArray = InArray;
	}

	friend FArchive& operator<<(FArchive& Ar, FInventorySelectionEntry& Value) { return Ar << Value.ConfigArray; }

private:
	UPROPERTY()
	TArray<TSoftClassPtr<UInventory>> ConfigArray = TArray<TSoftClassPtr<UInventory>>();

public:
	const TArray<TSoftClassPtr<UInventory>>& GetSoftClassList() const { return ConfigArray; }

	TArray<TSubclassOf<UInventory>> GetInventorySelection() const;

	static const TArray<TSoftClassPtr<UInventory>> InvalidConfigArray;
};

USTRUCT()
struct NAUSEA_API FPlayerClassSelectionMap
{
	GENERATED_USTRUCT_BODY()

public:
	FPlayerClassSelectionMap() {}

	FPlayerClassSelectionMap(const FPlayerClassSelectionMap& InSelectionMap)
	{
		VariantSelectionMap = InSelectionMap.VariantSelectionMap;
	}

	FPlayerClassSelectionMap(const TMap<EPlayerClassVariant, FInventorySelectionEntry>& InMap)
	{
		VariantSelectionMap = InMap;
	}

	friend FArchive& operator<<(FArchive& Ar, FPlayerClassSelectionMap& Entry);

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

public:
	FInventorySelectionEntry& operator[] (EPlayerClassVariant Variant) { return VariantSelectionMap[Variant]; }
	const FInventorySelectionEntry& operator[] (EPlayerClassVariant Variant) const { return VariantSelectionMap[Variant]; }

	FORCEINLINE bool Contains(EPlayerClassVariant Variant) const { return VariantSelectionMap.Contains(Variant); }
	FORCEINLINE int32 Remove(EPlayerClassVariant Variant) { return VariantSelectionMap.Remove(Variant); }
	FORCEINLINE FInventorySelectionEntry& FindOrAdd(EPlayerClassVariant Variant) { return VariantSelectionMap.FindOrAdd(Variant); }

protected:
	UPROPERTY()
	TMap<EPlayerClassVariant, FInventorySelectionEntry> VariantSelectionMap = TMap<EPlayerClassVariant, FInventorySelectionEntry>();
};
template<>
struct TStructOpsTypeTraits<FPlayerClassSelectionMap> : public TStructOpsTypeTraitsBase2<FPlayerClassSelectionMap>
{
	enum
	{
		WithNetSerializer = true
	};
};

USTRUCT(BlueprintType)
struct FPlayerSelectionStruct
{
	GENERATED_USTRUCT_BODY()

	FORCEINLINE const TMap<TSoftClassPtr<UPlayerClassComponent>, FPlayerClassSelectionMap>& GetInventorySelection() const { return InventorySelectionMap; }
	FORCEINLINE TMap<TSoftClassPtr<UPlayerClassComponent>, FPlayerClassSelectionMap>& GetInventorySelectionMutable() { return InventorySelectionMap; }

	FORCEINLINE const TSoftClassPtr<UPlayerClassComponent>& GetSelectedPlayerClass() const { return SelectedPlayerClass; }
	bool SetSelectedPlayerClass(TSoftClassPtr<UPlayerClassComponent> InSelectedPlayerClass);

	FORCEINLINE const TMap<TSoftClassPtr<UPlayerClassComponent>, EPlayerClassVariant>& GetPlayerClassVariantSelection() const { return SelectedPlayerClassVariantMap; }
	FORCEINLINE TMap<TSoftClassPtr<UPlayerClassComponent>, EPlayerClassVariant>& GetPlayerClassVariantSelectionMutable() { return SelectedPlayerClassVariantMap; }
	
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	UPROPERTY(NotReplicated)
	TMap<TSoftClassPtr<UPlayerClassComponent>, FPlayerClassSelectionMap> InventorySelectionMap = TMap<TSoftClassPtr<UPlayerClassComponent>, FPlayerClassSelectionMap>();

	UPROPERTY()
	TSoftClassPtr<UPlayerClassComponent> SelectedPlayerClass = nullptr;

	UPROPERTY(NotReplicated)
	TMap<TSoftClassPtr<UPlayerClassComponent>, EPlayerClassVariant> SelectedPlayerClassVariantMap = TMap<TSoftClassPtr<UPlayerClassComponent>, EPlayerClassVariant>();
};
template<>
struct TStructOpsTypeTraits<FPlayerSelectionStruct> : public TStructOpsTypeTraitsBase2<FPlayerSelectionStruct>
{
	enum
	{
		WithNetSerializer = true
	};
};

USTRUCT(BlueprintType)
struct FPlayerLocalDataStruct
{
	GENERATED_USTRUCT_BODY()

	FORCEINLINE const TSet<TSoftClassPtr<UStatusEffectBase>>& GetReceivedStatusEffects() const { return ReceivedStatusEffectMap; }
	FORCEINLINE bool HasReceivedStatusEffect(TSoftClassPtr<UStatusEffectBase> StatusEffectSoftClass) const { return ReceivedStatusEffectMap.Contains(StatusEffectSoftClass); }
	FORCEINLINE FSetElementId MarkStatusEffectReceived(TSoftClassPtr<UStatusEffectBase> StatusEffectSoftClass) { return ReceivedStatusEffectMap.Add(StatusEffectSoftClass); }

	//Never store but keep track here.
	UPROPERTY(Transient)
	TSet<TSoftClassPtr<UStatusEffectBase>> ReceivedStatusEffectMap = TSet<TSoftClassPtr<UStatusEffectBase>>();
};

UCLASS()
class NAUSEA_API UPlayerStatisticHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION()
	static bool IncrementPlayerStatistic(TScriptInterface<IPlayerOwnershipInterface> PlayerOwnedInterface, EPlayerStatisticType StatisticType, float Amount, bool bImmediatelyStore = false);

	UFUNCTION()
	static bool IncrementPlayerExperience(TScriptInterface<IPlayerOwnershipInterface> PlayerOwnedInterface, TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, uint64 Delta);

	UFUNCTION()
	static uint64 GetPlayerClassExperience(TScriptInterface<IPlayerOwnershipInterface> PlayerOwnedInterface, TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant);

	UFUNCTION()
	static bool CheckAndMarkStatusEffectReceived(TScriptInterface<IPlayerOwnershipInterface> PlayerOwnedInterface, TSubclassOf<UStatusEffectBase> StatusEffectClass);
};