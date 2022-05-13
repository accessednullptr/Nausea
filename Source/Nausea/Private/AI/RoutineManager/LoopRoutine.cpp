// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "AI/RoutineManager/LoopRoutine.h"
#include "AI/RoutineManager/Routine.h"

ULoopRoutine::ULoopRoutine(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bEndRoutineOnActionComplete = false;
}

void ULoopRoutine::StartRoutine()
{
	Super::StartRoutine();

	LoopCount = 0;
	StartNextRoutineAction();
}

void ULoopRoutine::RoutineActionCompleted(URoutineAction* RoutineAction)
{
	Super::RoutineActionCompleted(RoutineAction);

	//Might be weird to do this but we are intentionally delaying this by a frame so that infinite recursion does not happen.
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ULoopRoutine::StartNextRoutineAction);
}

void ULoopRoutine::StartNextRoutineAction()
{
	TSubclassOf<URoutineAction> NextRoutineActionClass = GetNextRoutineAction();
	if (!NextRoutineActionClass)
	{
		switch (NextRoutineNullResponse)
		{
		case ENextRoutineNullResponse::EndRoutine:
			EndRoutine();
			break;
		case ENextRoutineNullResponse::RetryNextFrame:
			GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ULoopRoutine::StartNextRoutineAction);
			break;
		}
		return;
	}

	CreateRoutineAction(NextRoutineActionClass);
	LoopCount++;
}

TSubclassOf<URoutineAction> ULoopRoutine::GetNextRoutineAction()
{
	return DefaultRoutineAction;
}