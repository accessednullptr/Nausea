// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "NauseaLevelScriptActor.generated.h"

class UMissionComponent;
class UObjective;

/**
 * 
 */
UCLASS()
class NAUSEA_API ANauseaLevelScriptActor : public ALevelScriptActor
{
	GENERATED_UCLASS_BODY()
	
//~ Begin AActor Interface
public:
	virtual void OnConstruction(const FTransform& Transform) override;
//~ End AActor Interface

public:
	UFUNCTION(BlueprintCallable, Category = LevelScriptActor)
	UMissionComponent* GetFirstMission() { return MissionList.IsValidIndex(0) ? MissionList[0] : GenerateLevelMissions(); }

protected:
	//Generates level missions. Returns starting missing.
	UFUNCTION(BlueprintImplementableEvent, Category = LevelScriptActor)
	UMissionComponent* GenerateLevelMissions();

protected:
	UPROPERTY()
	TArray<UMissionComponent*> MissionList;
	UPROPERTY()
	TArray<UObjective*> ObjectiveList;
};
