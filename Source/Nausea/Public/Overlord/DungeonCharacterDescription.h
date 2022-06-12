// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DungeonCharacterDescription.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, AutoExpandCategories = (Description))
class NAUSEA_API UDungeonCharacterDescription : public UObject
{
	GENERATED_UCLASS_BODY()
	
public:
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = Description)
	static const FText& GetCharacterName(TSubclassOf<UDungeonCharacterDescription> Class);

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = Description)
	static const TSoftObjectPtr<UTexture2D>& GetPortraitTexture(TSubclassOf<UDungeonCharacterDescription> Class);
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = Description)
	static const TSoftObjectPtr<UTexture2D>& GetFrameTexture(TSubclassOf<UDungeonCharacterDescription> Class);
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = Description)
	static const TSoftObjectPtr<UTexture2D>& GetBackplateTexture(TSubclassOf<UDungeonCharacterDescription> Class);

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = Description)
	static const TSoftObjectPtr<UMaterialInterface>& GetPortraitMaterialOverride(TSubclassOf<UDungeonCharacterDescription> Class);
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = Description)
	static const TSoftObjectPtr<UMaterialInterface>& GetFrameMaterialOverride(TSubclassOf<UDungeonCharacterDescription> Class);
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = Description)
	static const TSoftObjectPtr<UMaterialInterface>& GetBackplateMaterialOverride(TSubclassOf<UDungeonCharacterDescription> Class);

protected:
	UPROPERTY(EditDefaultsOnly, Category = Description)
	FText CharacterName = FText();

	UPROPERTY(EditDefaultsOnly, Category = Description)
	TSoftObjectPtr<UTexture2D> PortraitTexture = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Description)
	TSoftObjectPtr<UTexture2D> FrameTexture = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Description)
	TSoftObjectPtr<UTexture2D> BackplateTexture = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = Description)
	TSoftObjectPtr<UMaterialInterface> PortraitMaterialOverride = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Description)
	TSoftObjectPtr<UMaterialInterface> FrameMaterialOverride = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Description)
	TSoftObjectPtr<UMaterialInterface> BackplateMaterialOverride = nullptr;
};
