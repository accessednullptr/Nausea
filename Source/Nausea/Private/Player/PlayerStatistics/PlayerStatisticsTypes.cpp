// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerStatistics/PlayerStatisticsTypes.h"
#include "Player/PlayerOwnershipInterface.h"
#include "Player/PlayerStatistics/PlayerStatisticsComponent.h"
#include "Player/PlayerClassComponent.h"
#include "Weapon/Inventory.h"

const TMap<TSoftClassPtr<UPlayerClassComponent>, uint64> FExperienceStruct::InvalidExperienceStruct = TMap<TSoftClassPtr<UPlayerClassComponent>, uint64>();

const uint64 FVariantExperienceMap::InvalidExperienceValue = 0;
const FVariantExperienceMap FVariantExperienceMap::InvalidVariantExperienceMap = FVariantExperienceMap();

const TArray<TSoftClassPtr<UInventory>> FInventorySelectionEntry::InvalidConfigArray = TArray<TSoftClassPtr<UInventory>>();

typedef TMapPairStruct<EPlayerClassVariant, uint64> FVariantExperiencePairStruct;

FArchive& operator<<(FArchive& Ar, FVariantExperienceMap& Entry)
{
	FVariantExperiencePairStruct::NetSerializeTemplateMap(Ar, Entry.VariantExperienceMap);
	return Ar;
}

bool FVariantExperienceMap::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;
	return FVariantExperiencePairStruct::NetSerializeTemplateMap(Ar, VariantExperienceMap);
}

const uint64& FVariantExperienceMap::GetValue(EPlayerClassVariant Variant) const
{
	if (VariantExperienceMap.Contains(Variant))
	{
		return VariantExperienceMap[Variant];
	}

	return FVariantExperienceMap::InvalidExperienceValue;
}

uint64 FVariantExperienceMap::AddValue(EPlayerClassVariant Variant, uint64 Delta)
{
	return (VariantExperienceMap.FindOrAdd(Variant) += Delta);
}

uint64 FVariantExperienceMap::SetValue(EPlayerClassVariant Variant, uint64 InValue)
{
	return (VariantExperienceMap.FindOrAdd(Variant) = InValue);
}

typedef TMapPairStruct<TSoftClassPtr<UPlayerClassComponent>, FVariantExperienceMap> FExperiencePairStruct;
bool FExperienceStruct::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;
	return FExperiencePairStruct::NetSerializeTemplateMap(Ar, PlayerClassExperienceMap);
}

const FVariantExperienceMap& FExperienceStruct::operator[] (TSoftClassPtr<UPlayerClassComponent> PlayerClass) const
{
	if (PlayerClassExperienceMap.Contains(PlayerClass))
	{
		return PlayerClassExperienceMap[PlayerClass];
	}

	return FVariantExperienceMap::InvalidVariantExperienceMap;
}

bool FExperienceStruct::IsValid(const FVariantExperienceMap& ExperienceValue) const
{
	return &ExperienceValue != &FVariantExperienceMap::InvalidVariantExperienceMap;
}

const uint64& FExperienceStruct::GetValue(TSoftClassPtr<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant) const
{
	if (PlayerClassExperienceMap.Contains(PlayerClass))
	{
		return PlayerClassExperienceMap[PlayerClass].GetValue(Variant);
	}

	return FVariantExperienceMap::InvalidExperienceValue;
}

uint64 FExperienceStruct::AddValue(TSoftClassPtr<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, uint64 Delta)
{
	FVariantExperienceMap& ExperienceMap = PlayerClassExperienceMap.FindOrAdd(PlayerClass);
	ExperienceMap.FindOrAdd(UPlayerClassHelpers::GetOppositeVariant(Variant)) += Delta / 2;
	return ExperienceMap.FindOrAdd(Variant) += Delta;
}

uint64 FExperienceStruct::SetValue(TSoftClassPtr<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, uint64 InValue)
{
	return (PlayerClassExperienceMap.FindOrAdd(PlayerClass).SetValue(Variant, InValue));
}

bool FExperienceStruct::ReplaceWithLarger(const FExperienceStruct& Other)
{
	TArray<TSoftClassPtr<UPlayerClassComponent>> OtherKeyList;
	Other.Get().GenerateKeyArray(OtherKeyList);

	for (TSoftClassPtr<UPlayerClassComponent> Key : OtherKeyList)
	{
		TArray<EPlayerClassVariant> OtherVariantKeyList;
		Other[Key].Get().GenerateKeyArray(OtherVariantKeyList);

		for (EPlayerClassVariant VariantKey : OtherVariantKeyList)
		{
			SetValue(Key, VariantKey, FMath::Max(GetValue(Key, VariantKey), Other[Key][VariantKey]));
		}
	}

	return true;
}

typedef TMapPairStruct<EPlayerStatisticType, uint64> FPlayerStatisticsPairStruct;
bool FPlayerStatisticsStruct::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;
	return FPlayerStatisticsPairStruct::NetSerializeTemplateMap(Ar, PlayerStatisticMap);
}

bool FPlayerStatisticsStruct::ReplaceWithLarger(const FPlayerStatisticsStruct& Other)
{
	TArray<EPlayerStatisticType> OtherKeyList;
	Other.Get().GenerateKeyArray(OtherKeyList);

	for (EPlayerStatisticType Key : OtherKeyList)
	{
		Set(Key, FMath::Max(Get(Key), Other.Get(Key)));
	}

	return true;
}

TArray<TSubclassOf<UInventory>> FInventorySelectionEntry::GetInventorySelection() const
{
	TArray<TSubclassOf<UInventory>> InventorySelection;
	InventorySelection.Reserve(ConfigArray.Num());

	for (const TSoftClassPtr<UInventory>& ConfigClass : ConfigArray)
	{
		TSubclassOf<UInventory> InventoryClass = ConfigClass.Get();

		if (!InventoryClass)
		{
			continue;
		}

		InventorySelection.Add(InventoryClass);
	}

	return InventorySelection;
}

typedef TMapPairStruct<EPlayerClassVariant, FInventorySelectionEntry> FPlayerClassSelectionMapStruct;

//Because is a nested TMap within a TMap, we need to implement the operator again like this.
FArchive& operator<<(FArchive& Ar, FPlayerClassSelectionMap& Entry)
{
	FPlayerClassSelectionMapStruct::NetSerializeTemplateMap(Ar, Entry.VariantSelectionMap);
	return Ar;
}

bool FPlayerClassSelectionMap::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;
	return FPlayerClassSelectionMapStruct::NetSerializeTemplateMap(Ar, VariantSelectionMap);
}

bool FPlayerSelectionStruct::SetSelectedPlayerClass(TSoftClassPtr<UPlayerClassComponent> InSelectedPlayerClass)
{
	bool bChanged = InSelectedPlayerClass != SelectedPlayerClass;
	SelectedPlayerClass = InSelectedPlayerClass;
	return bChanged;
}

typedef TMapPairStruct<TSoftClassPtr<UPlayerClassComponent>, FPlayerClassSelectionMap> FPlayerSelectionPairStruct;
typedef TMapPairStruct<TSoftClassPtr<UPlayerClassComponent>, EPlayerClassVariant> FPlayerVariantSelectionPairStruct;
bool FPlayerSelectionStruct::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;
	FPlayerSelectionPairStruct::NetSerializeTemplateMap(Ar, InventorySelectionMap);
	Ar << SelectedPlayerClass;
	FPlayerVariantSelectionPairStruct::NetSerializeTemplateMap(Ar, SelectedPlayerClassVariantMap);
	return true;
}

UPlayerStatisticHelpers::UPlayerStatisticHelpers(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

bool UPlayerStatisticHelpers::IncrementPlayerStatistic(TScriptInterface<IPlayerOwnershipInterface> PlayerOwnedInterface, EPlayerStatisticType StatisticType, float Amount, bool bImmediatelyStore)
{
	if (!PlayerOwnedInterface || !PlayerOwnedInterface->GetPlayerStatisticsComponent())
	{
		return false;
	}

	if (PlayerOwnedInterface->GetPlayerStatisticsComponent()->GetOwnerRole() != ROLE_Authority)
	{
		return false;
	}

	PlayerOwnedInterface->GetPlayerStatisticsComponent()->UpdatePlayerStatistic(StatisticType, Amount);
	return true;
}

bool UPlayerStatisticHelpers::IncrementPlayerExperience(TScriptInterface<IPlayerOwnershipInterface> PlayerOwnedInterface, TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, uint64 Delta)
{
	if (!PlayerOwnedInterface || !PlayerOwnedInterface->GetPlayerStatisticsComponent())
	{
		return false;
	}

	if (PlayerOwnedInterface->GetPlayerStatisticsComponent()->GetOwnerRole() != ROLE_Authority)
	{
		return false;
	}

	PlayerOwnedInterface->GetPlayerStatisticsComponent()->UpdatePlayerExperience(PlayerClass, Variant, Delta);
	return true;
}

uint64 UPlayerStatisticHelpers::GetPlayerClassExperience(TScriptInterface<IPlayerOwnershipInterface> PlayerOwnedInterface, TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant)
{
	if (!PlayerOwnedInterface || !PlayerOwnedInterface->GetPlayerStatisticsComponent())
	{
		return 0;
	}

	if (!PlayerClass || (Variant != EPlayerClassVariant::Primary && Variant != EPlayerClassVariant::Alternative))
	{
		return 0;
	}

	return PlayerOwnedInterface->GetPlayerStatisticsComponent()->GetPlayerClassExperience(PlayerClass, Variant);
}

bool UPlayerStatisticHelpers::CheckAndMarkStatusEffectReceived(TScriptInterface<IPlayerOwnershipInterface> PlayerOwnedInterface, TSubclassOf<UStatusEffectBase> StatusEffectClass)
{
	if (!PlayerOwnedInterface || !PlayerOwnedInterface->GetPlayerStatisticsComponent())
	{
		return false;
	}

	return PlayerOwnedInterface->GetPlayerStatisticsComponent()->CheckAndMarkStatusEffectReceived(StatusEffectClass);
}