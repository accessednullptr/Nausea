// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "NauseaWorldSettings.generated.h"

class UMapDataAsset;

/**
 * 
 */
UCLASS()
class NAUSEA_API ANauseaWorldSettings : public AWorldSettings
{
	GENERATED_UCLASS_BODY()
	
public:
	UFUNCTION()
	const TSoftObjectPtr<UMapDataAsset>& GetMapDataAsset() const { return MapDataAsset; }
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WorldSettings)
	TSoftObjectPtr<UMapDataAsset> MapDataAsset;
};
