// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "System/MapDataAsset.h"
#include "Internationalization/StringTableRegistry.h"

const FPrimaryAssetType UMapDataAsset::MapDataAssetType = FName(TEXT("MapDataAsset"));

UMapDataAsset::UMapDataAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

FText UMapDataAsset::GetGameTypeTextForEnum(EGameType GameType)
{
	switch (GameType)
	{
	case EGameType::Raid:
		return LOCTABLE("/Game/Localization/GenericStringTable.GenericStringTable", "GameType_Raid");
	case EGameType::Survival:
		return LOCTABLE("/Game/Localization/GenericStringTable.GenericStringTable", "GameType_Survival");
	case EGameType::Custom:
		return LOCTABLE("/Game/Localization/GenericStringTable.GenericStringTable", "GameType_Custom");
	}

	return LOCTABLE("/Game/Localization/GenericStringTable.GenericStringTable", "GameType_Invalid");
}

FString UMapDataAsset::GetMapTravelName() const
{
	if (MapAsset.IsNull())
	{
		return FString("");
	}

	FString MapTravelName;
	MapAsset.ToString().Split(".", nullptr, &MapTravelName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	return MapTravelName;
}