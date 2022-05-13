// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Player/CorePlayerCameraManager.h"
#include "NauseaPlayerCameraManager.generated.h"

class AActor;
class APlayerController;
class ACorePlayerController;

/**
 * 
 */
UCLASS()
class NAUSEA_API ANauseaPlayerCameraManager : public ACorePlayerCameraManager
{
	GENERATED_UCLASS_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = PlayerCameraManager)
	ANauseaPlayerController* GetNauseaPlayerController() const;
};
