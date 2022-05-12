// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "AI/Action/ActionRepeat.h"

UActionRepeat::UActionRepeat(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SubActionTriggeringPolicy = EPawnSubActionTriggeringPolicy::CopyBeforeTriggering;
	ChildFailureHandlingMode = EPawnActionFailHandling::IgnoreFailure;
}

UActionRepeat* UActionRepeat::CreateRepeatAction(const UObject* WorldContextObject, UActionBrainComponentAction* Action, int32 NumberOfRepeats, TEnumAsByte<EPawnActionFailHandling::Type> FailureHandlingMode)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World)
	{
		return nullptr;
	}

	if (Action == nullptr || !(NumberOfRepeats > 0 || NumberOfRepeats == UActionRepeat::LoopForever))
	{
		return nullptr;
	}

	UActionRepeat* RepeatAction = UActionBrainComponentAction::CreateActionInstance<UActionRepeat>(World);
	if (RepeatAction)
	{
		RepeatAction->ActionToRepeat = Action;
		RepeatAction->RepeatCount = NumberOfRepeats;
		RepeatAction->ChildFailureHandlingMode = FailureHandlingMode;
		RepeatAction->bShouldPauseMovement = Action->ShouldPauseMovement();
	}

	return RepeatAction;
}

bool UActionRepeat::Start()
{
	bool bResult = Super::Start();

	if (bResult)
	{
		UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("Starting repeating action: %s. Requested repeats: %d")
			, *GetNameSafe(ActionToRepeat), RepeatCount);
		bResult = PushSubAction();
	}

	return bResult;
}

bool UActionRepeat::Resume()
{
	bool bResult = Super::Resume();

	if (bResult)
	{
		bResult = PushSubAction();
	}

	return bResult;
}

void UActionRepeat::OnChildFinished(UActionBrainComponentAction* Action, EPawnActionResult::Type WithResult)
{
	Super::OnChildFinished(Action, WithResult);

	if (RecentActionCopy == Action)
	{
		if (WithResult == EPawnActionResult::Success || (WithResult == EPawnActionResult::Failed && ChildFailureHandlingMode == EPawnActionFailHandling::IgnoreFailure))
		{
			PushSubAction();
		}
		else
		{
			Finish(EPawnActionResult::Failed);
		}
	}
}

void UActionRepeat::Finish(TEnumAsByte<EPawnActionResult::Type> WithResult)
{
	RecentActionCopy = nullptr;
	Super::Finish(WithResult);
}

FString UActionRepeat::GetDebugInfoString(int32 Depth) const
{
	return Super::GetDebugInfoString(Depth);
}

bool UActionRepeat::PushSubAction()
{
	if (ActionToRepeat == NULL)
	{
		Finish(EPawnActionResult::Failed);
		return false;
	}
	else if (RepeatCount == 0)
	{
		Finish(EPawnActionResult::Success);
		return true;
	}

	if (RepeatCount > 0)
	{
		--RepeatCount;
	}

	UActionBrainComponentAction* ActionCopy = SubActionTriggeringPolicy == EPawnSubActionTriggeringPolicy::CopyBeforeTriggering
		? Cast<UActionBrainComponentAction>(StaticDuplicateObject(ActionToRepeat, this))
		: ActionToRepeat;

	UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("%s> pushing repeated action %s %s, repeats left: %d")
		, *GetName(), SubActionTriggeringPolicy == EPawnSubActionTriggeringPolicy::CopyBeforeTriggering ? TEXT("copy") : TEXT("instance")
		, *GetNameSafe(ActionCopy), RepeatCount);
	check(ActionCopy);
	RecentActionCopy = ActionCopy;
	return PushChildAction(ActionCopy); 
}
