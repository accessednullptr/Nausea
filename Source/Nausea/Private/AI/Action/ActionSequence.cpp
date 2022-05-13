// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#include "AI/Action/ActionSequence.h"
#include "Engine/World.h"
#include "AIController.h"
#include "VisualLogger/VisualLogger.h"

UActionSequence::UActionSequence(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SubActionTriggeringPolicy = EPawnSubActionTriggeringPolicy::CopyBeforeTriggering;
	ChildFailureHandlingMode = EPawnActionFailHandling::RequireSuccess;

	CurrentActionIndex = 0;
}

UActionSequence* UActionSequence::CreateSequenceAction(const UObject* WorldContextObject, TArray<UActionBrainComponentAction*> Actions, TEnumAsByte<EPawnActionFailHandling::Type> FailureHandlingMode)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World)
	{
		return nullptr;
	}

	if (Actions.Num() == 0)
	{
		return nullptr;
	}

	UActionSequence* Action = UActionBrainComponentAction::CreateActionInstance<UActionSequence>(World);
	if (Action)
	{
		Action->ActionSequence = MoveTemp(Actions);
	}

	return Action;
}

bool UActionSequence::Start()
{
	bool bResult = Super::Start();

	if (bResult)
	{
		UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("Starting sequence. Items:"), *GetName());
		for (UActionBrainComponentAction* Action : ActionSequence)
		{
			UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("    %s"), *GetNameSafe(Action));
		}

		bResult = PushNextActionCopy();
	}

	return bResult;
}

bool UActionSequence::Resume()
{
	bool bResult = Super::Resume();

	if (bResult)
	{
		bResult = PushNextActionCopy();
	}

	return bResult;
}

void UActionSequence::OnChildFinished(UActionBrainComponentAction* Action, EPawnActionResult::Type WithResult)
{
	Super::OnChildFinished(Action, WithResult);

	if (RecentActionCopy == Action)
	{
		if (WithResult == EPawnActionResult::Success || (WithResult == EPawnActionResult::Failed && ChildFailureHandlingMode == EPawnActionFailHandling::IgnoreFailure))
		{
			if (GetAbortState() == EPawnActionAbortState::NotBeingAborted)
			{
				PushNextActionCopy();
			}
		}
		else
		{
			Finish(EPawnActionResult::Failed);
		}
	}
}

void UActionSequence::Finish(TEnumAsByte<EPawnActionResult::Type> WithResult)
{
	RecentActionCopy = nullptr;
	Super::Finish(WithResult);
}

FString UActionSequence::GetDebugInfoString(int32 Depth) const
{
	FString String = FString::ChrN(Depth * 4, ' ') + FString::Printf(TEXT("%d. %s [%s]\n"), Depth, *GetDisplayName(), *GetStateDescription());

	for (int32 Index = 0; Index < ActionSequence.Num(); Index++)
	{
		if (Index == CurrentActionIndex - 1 && ChildAction)
		{
			String += ChildAction->GetDebugInfoString(Depth + 1);
			continue;
		}

		if (ActionSequence[Index])
		{
			String += ActionSequence[Index]->GetDebugInfoString(Depth + 1);
		}
		else
		{
			String += GetNoneActionInfoString(Depth + 1);
		}
	}

	return String;
}

bool UActionSequence::PushNextActionCopy()
{
	if (CurrentActionIndex >= uint32(ActionSequence.Num()))
	{
		Finish(EPawnActionResult::Success);
		return true;
	}

	UActionBrainComponentAction* ActionCopy = SubActionTriggeringPolicy == EPawnSubActionTriggeringPolicy::CopyBeforeTriggering
		? Cast<UActionBrainComponentAction>(StaticDuplicateObject(ActionSequence[CurrentActionIndex], this))
		: ActionSequence[CurrentActionIndex];

	UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("%s> pushing action %s")
		, *GetName(), *GetNameSafe(ActionCopy));
	++CurrentActionIndex;	
	check(ActionCopy);
	RecentActionCopy = ActionCopy;
	return PushChildAction(ActionCopy);
}
