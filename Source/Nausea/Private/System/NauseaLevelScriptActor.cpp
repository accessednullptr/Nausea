// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#include "System/NauseaLevelScriptActor.h"
#include "System/NauseaGameInstance.h"
#include "System/NauseaGameState.h"
#include "Objective/MissionComponent.h"
#include "Objective/Objective.h"

ANauseaLevelScriptActor::ANauseaLevelScriptActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void ANauseaLevelScriptActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	return;

	TArray<UMissionComponent*> MissionComponentList;
	TMap<UMissionComponent*, TArray<UObjective*>> ObjectiveMap;
	
	UClass* Class = GetClass();

	for (UMissionComponent* Mission : MissionList)
	{
		if (!Mission)
		{
			continue;
		}

		Mission->DestroyComponent();
	}
	MissionList.Reset();

	for (UObjective* Objective : ObjectiveList)
	{
		if (!Objective)
		{
			continue;
		}

		Objective->MarkPendingKill();
	}
	ObjectiveList.Reset();

	UMissionComponent* MissionComponent = GenerateLevelMissions();
	while (MissionComponent)
	{
		MissionComponentList.Add(MissionComponent);
		ObjectiveMap.Add(MissionComponent) = MissionComponent->GetObjectiveList();

		MissionComponent = MissionComponent->GetFollowingMission();
	}

	MissionList = MissionComponentList;

	TArray<TArray<UObjective*>> MissionObjectiveList;
	ObjectiveMap.GenerateValueArray(MissionObjectiveList);
	for (const TArray<UObjective*>& List : MissionObjectiveList)
	{
		ObjectiveList.Append(List);
	}
}