// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Overlord/DungeonCharacterDescription.h"

UDungeonCharacterDescription::UDungeonCharacterDescription(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

static FText InvalidName = FText::GetEmpty();
const FText& UDungeonCharacterDescription::GetCharacterName(TSubclassOf<UDungeonCharacterDescription> Class)
{
	if (!Class)
	{
		return InvalidName;
	}

	return Class.GetDefaultObject()->CharacterName;
}

static TSoftObjectPtr<UTexture2D> InvalidTexture = nullptr;
const TSoftObjectPtr<UTexture2D>& UDungeonCharacterDescription::GetPortraitTexture(TSubclassOf<UDungeonCharacterDescription> Class)
{
	if (!Class)
	{
		return InvalidTexture;
	}

	return Class.GetDefaultObject()->PortraitTexture;
}

const TSoftObjectPtr<UTexture2D>& UDungeonCharacterDescription::GetFrameTexture(TSubclassOf<UDungeonCharacterDescription> Class)
{
	if (!Class)
	{
		return InvalidTexture;
	}

	return Class.GetDefaultObject()->FrameTexture;
}

const TSoftObjectPtr<UTexture2D>& UDungeonCharacterDescription::GetBackplateTexture(TSubclassOf<UDungeonCharacterDescription> Class)
{
	if (!Class)
	{
		return InvalidTexture;
	}

	return Class.GetDefaultObject()->BackplateTexture;
}

static TSoftObjectPtr<UMaterialInterface> InvalidMaterial = nullptr;
const TSoftObjectPtr<UMaterialInterface>& UDungeonCharacterDescription::GetPortraitMaterialOverride(TSubclassOf<UDungeonCharacterDescription> Class)
{
	if (!Class)
	{
		return InvalidMaterial;
	}

	return Class.GetDefaultObject()->PortraitMaterialOverride;
}

const TSoftObjectPtr<UMaterialInterface>& UDungeonCharacterDescription::GetFrameMaterialOverride(TSubclassOf<UDungeonCharacterDescription> Class)
{
	if (!Class)
	{
		return InvalidMaterial;
	}

	return Class.GetDefaultObject()->FrameMaterialOverride;
}

const TSoftObjectPtr<UMaterialInterface>& UDungeonCharacterDescription::GetBackplateMaterialOverride(TSubclassOf<UDungeonCharacterDescription> Class)
{
	if (!Class)
	{
		return InvalidMaterial;
	}

	return Class.GetDefaultObject()->BackplateMaterialOverride;
}