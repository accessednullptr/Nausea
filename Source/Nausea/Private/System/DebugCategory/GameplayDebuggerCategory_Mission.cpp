// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "System/DebugCategory/GameplayDebuggerCategory_Mission.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "GameFramework/PlayerController.h"
#include "System/NauseaGameState.h"
#include "Objective/MissionComponent.h"
#include "Engine/Canvas.h"

FGameplayDebuggerCategory_Mission::FGameplayDebuggerCategory_Mission()
{
	bShowOnlyWithDebugActor = false;
	SetDataPackReplication<FRepData>(&DataPack);
}

void FGameplayDebuggerCategory_Mission::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
	if (!OwnerPC || !OwnerPC->GetWorld())
	{
		return;
	}

	ANauseaGameState* NauseaGameState = OwnerPC->GetWorld()->GetGameState<ANauseaGameState>();

	if (!NauseaGameState)
	{
		return;
	}

	UMissionComponent* MissionComponent = NauseaGameState->GetInitialMission();

	if (!MissionComponent)
	{
		return;
	}

	int32 Index = 0;
	while (MissionComponent)
	{
		Index++;

		DataPack.MissionDescriptionList.Add(MissionComponent->DescribeMissionToGameplayDebugger(Index));
		DataPack.ObjectiveDescriptionList.Append(FRepData::FObjectiveData::GetArrayFromDescriptionList(Index, MissionComponent->DescribeObjectivesToGameplayDebugger(Index)));
		MissionComponent = MissionComponent->GetFollowingMission();
	}
}

void FGameplayDebuggerCategory_Mission::DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext)
{
	FVector2D ViewPortSize;
	GEngine->GameViewport->GetViewportSize(ViewPortSize);

	const FVector2D BackgroundSize((ViewPortSize.X * 0.25f) - 4.f, ViewPortSize.Y);
	const FLinearColor BackgroundColor(0.01f, 0.01f, 0.01f, 0.8f);

	CanvasContext.Printf(TEXT("{green}Mission List:"));

	if (DataPack.MissionDescriptionList.Num() == 0)
	{
		CanvasContext.Printf(TEXT("{white}None"));
		return;
	}

	const TArray<FString>& MissionDescriptionList = DataPack.MissionDescriptionList;
	const TArray<FRepData::FObjectiveData>& ObjectiveDescriptionList = DataPack.ObjectiveDescriptionList;
	int32 ObjectiveDescriptionIndex = 0;
	int32 MissionIndexIndex = 0;

	for (const FString& MissionDescription : MissionDescriptionList)
	{
		MissionIndexIndex++;
		CanvasContext.Printf(TEXT("%s"), *MissionDescription);

		while (ObjectiveDescriptionList.IsValidIndex(ObjectiveDescriptionIndex) && ObjectiveDescriptionList[ObjectiveDescriptionIndex].MissionIndex == MissionIndexIndex)
		{
			CanvasContext.Printf(TEXT("%s"), *ObjectiveDescriptionList[ObjectiveDescriptionIndex].ObjectiveDescription);
			ObjectiveDescriptionIndex++;
		}
	}
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_Mission::MakeInstance()
{
	return MakeShareable(new FGameplayDebuggerCategory_Mission());
}

void FGameplayDebuggerCategory_Mission::FRepData::Serialize(FArchive& Ar)
{
	int32 NumMissions = MissionDescriptionList.Num();
	Ar << NumMissions;
	if (Ar.IsLoading())
	{
		MissionDescriptionList.SetNum(NumMissions);
	}

	for (int32 Idx = 0; Idx < NumMissions; Idx++)
	{
		Ar << MissionDescriptionList[Idx];
	}

	int32 NumObjectives = ObjectiveDescriptionList.Num();
	Ar << NumObjectives;
	if (Ar.IsLoading())
	{
		ObjectiveDescriptionList.SetNum(NumObjectives);
	}

	for (int32 Idx = 0; Idx < NumObjectives; Idx++)
	{
		Ar << ObjectiveDescriptionList[Idx].MissionIndex;
		Ar << ObjectiveDescriptionList[Idx].ObjectiveDescription;
	}
}
#endif // WITH_GAMEPLAY_DEBUGGER