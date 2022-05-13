// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


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