// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UI/CoreUserWidget.h"
#include "MissionObjectiveUserWidget.generated.h"

/**
 * Used to display a specific objective's information.
 */
UCLASS()
class NAUSEA_API UMissionObjectiveUserWidget : public UCoreUserWidget
{
	GENERATED_UCLASS_BODY()
	
protected:
	UPROPERTY(BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	class UObjective* Objective = nullptr;
};
