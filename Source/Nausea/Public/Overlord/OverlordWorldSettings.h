// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "System/CoreWorldSettings.h"
#include "OverlordWorldSettings.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API AOverlordWorldSettings : public ACoreWorldSettings
{
	GENERATED_UCLASS_BODY()
	
public:
	UFUNCTION(CallInEditor, Category = "GridGeneration")
	void BuildAllGridData();
};
