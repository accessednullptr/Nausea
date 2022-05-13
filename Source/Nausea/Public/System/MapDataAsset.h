// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MapDataAsset.generated.h"

UENUM(BlueprintType)
enum class EGameType : uint8
{
	Raid,
	Survival,
	Custom,
	MainMenu,
	Invalid
};

USTRUCT(BlueprintType)
struct FObjectiveEntry
{
	GENERATED_USTRUCT_BODY()

	FObjectiveEntry() {}

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText ObjectiveName = FText();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText ObjectiveDescription = FText();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UTexture> ObjectiveIcon = nullptr;
};

/**
 * 
 */
UCLASS()
class NAUSEA_API UMapDataAsset : public UPrimaryDataAsset
{
	GENERATED_UCLASS_BODY()
	
public:
	static const FPrimaryAssetType MapDataAssetType;

	UFUNCTION(BlueprintCallable, Category = MapData)
	const TSoftObjectPtr<UWorld>& GetMapAsset() const { return MapAsset; }
	
	UFUNCTION(BlueprintCallable, Category = MapData)
	bool IsMainMenu() const { return bMainMenu; }

	UFUNCTION(BlueprintCallable, Category = MapData)
	const FText& GetMapName() const { return MapName; }
	UFUNCTION(BlueprintCallable, Category = MapData)
	const TSoftObjectPtr<UTexture>& GetMapIcon() const { return MapIcon; }
	
	UFUNCTION(BlueprintCallable, Category = MapData)
	const FText& GetMapDescription() const { return MapDescription; }

	UFUNCTION(BlueprintCallable, Category = MapData)
	const FText& GetMapShortDescription() const { return MapShortDescription; }

	UFUNCTION(BlueprintCallable, Category = MapData)
	EGameType GetGameType() const { return GameType; }

	UFUNCTION(BlueprintCallable, Category = MapData)
	FText GetGameTypeText() const { return GetGameTypeTextForEnum(GameType); }

	UFUNCTION(BlueprintCallable, Category = MapData)
	static FText GetGameTypeTextForEnum(EGameType GameType);

	UFUNCTION(BlueprintCallable, Category = MapData)
	const TArray<FObjectiveEntry>& GetObjectiveList() const { return ObjectiveList; }

	UFUNCTION(BlueprintCallable, Category = MapData)
	FString GetMapTravelName() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = MapData)
	TSoftObjectPtr<UWorld> MapAsset = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = MapData)
	bool bMainMenu = false;

	UPROPERTY(EditDefaultsOnly, Category = MapData)
	FText MapName = FText();

	UPROPERTY(EditDefaultsOnly, Category = MapData)
	FText MapShortDescription = FText();

	UPROPERTY(EditDefaultsOnly, Category = MapData)
	TSoftObjectPtr<UTexture> MapIcon = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = MapData)
	FText MapDescription = FText();
	
	UPROPERTY(EditDefaultsOnly, Category = MapData)
	EGameType GameType = EGameType::Raid;

	UPROPERTY(EditDefaultsOnly, Category = MapData)
	TArray<FObjectiveEntry> ObjectiveList = TArray<FObjectiveEntry>();
};