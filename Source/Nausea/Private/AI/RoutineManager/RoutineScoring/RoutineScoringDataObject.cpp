// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "AI/RoutineManager/RoutineScoring/RoutineScoringDataObject.h"

URoutineScoringDataObject::URoutineScoringDataObject(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void URoutineScoringDataObject::OnRoutineSelected(URoutine* Routine)
{
	OnRoutineScoringObjectSelected.Broadcast(this, Routine);
}

void URoutineScoringDataObject::OnRoutineCompleted(URoutine* Routine)
{
	OnRoutineScoringObjectCompleted.Broadcast(this, Routine);
}

UScoreObject::UScoreObject(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}