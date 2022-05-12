// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "AI/ActionStack.h"
#include "AI/ActionBrainComponent.h"
#include "AI/ActionBrainComponentAction.h"

void FActionStack::Pause()
{
	if (TopAction != NULL)
	{
		TopAction->Pause(NULL);
	}
}

void FActionStack::Resume()
{
	if (TopAction != NULL)
	{
		TopAction->Resume();
	}
}

void FActionStack::PushAction(UActionBrainComponentAction* NewTopAction)
{
	if (TopAction != NULL)
	{
		if (TopAction->IsPaused() == false && TopAction->HasBeenStarted() == true)
		{
			TopAction->Pause(NewTopAction);
		}

		ensure(TopAction->ChildAction == NULL);
		TopAction->ChildAction = NewTopAction;
		NewTopAction->ParentAction = TopAction;
	}

	TopAction = NewTopAction;
	TopAction->OnPushed();
}

void FActionStack::PopAction(UActionBrainComponentAction* ActionToPop)
{
	if (!ActionToPop)
	{
		return;
	}

	// first check if it's there
	UActionBrainComponentAction* CutPoint = TopAction;

	while (CutPoint != NULL && CutPoint != ActionToPop)
	{
		CutPoint = CutPoint->ParentAction;
	}

	if (CutPoint == ActionToPop)
	{
		UActionBrainComponentAction* ActionBeingRemoved = TopAction;
		// note StopAction can be null
		UActionBrainComponentAction* StopAction = ActionToPop->ParentAction;

		while (ActionBeingRemoved != StopAction && ActionBeingRemoved != nullptr)
		{
			checkSlow(ActionBeingRemoved);
			UActionBrainComponentAction* NextAction = ActionBeingRemoved->ParentAction;

			if (ActionBeingRemoved->IsBeingAborted() == false && ActionBeingRemoved->IsFinished() == false)
			{
				// forcing abort to make sure it happens instantly. We don't have time for delayed finish here.
				ActionBeingRemoved->Abort(EAIForceParam::Force);
			}
			
			ActionBeingRemoved->OnPopped();

			if (ActionBeingRemoved->ParentAction)
			{
				ActionBeingRemoved->ParentAction->OnChildFinished(ActionBeingRemoved, ActionBeingRemoved->FinishResult);
			}

			ActionBeingRemoved->CleanUp();

			ActionBeingRemoved = NextAction;
		}

		TopAction = StopAction;
	}
}

int32 FActionStack::GetStackSize() const
{
	int32 Size = 0;
	const UActionBrainComponentAction* TempAction = TopAction;
	while (TempAction != nullptr)
	{
		TempAction = TempAction->GetParentAction();
		++Size;
	}
	return Size;
}