// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "AI/Action/ActionWait.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "VisualLogger/VisualLogger.h"

UActionWait::UActionWait(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UActionWait* UActionWait::CreateWaitAction(const UObject* WorldContextObject, float InWaitDuration, float InWaitVariance)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World)
	{
		return nullptr;
	}

	UActionWait* WaitAction = UActionBrainComponentAction::CreateActionInstance<UActionWait>(World);

	if (!WaitAction)
	{
		return nullptr;
	}

	WaitAction->WaitDuration = InWaitDuration;
	WaitAction->WaitVariance = InWaitVariance;
	return WaitAction;
}

bool UActionWait::Start()
{
	if (Super::Start())
	{
		if (WaitDuration > 0)
		{
			GetWorld()->GetTimerManager().SetTimer(WaitTimerHandle, FTimerDelegate::CreateUObject(this, &UActionWait::WaitComplete), WaitDuration + FMath::RandRange(-WaitVariance, WaitVariance), false);
			UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("%s> Started wait for %f seconds."), *GetName(), GetWorld()->GetTimerManager().GetTimerRemaining(WaitTimerHandle));
		}
		else
		{
			UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("%s> Started indefinite wait. Something is going to call UActionWait::TimerComplete manually"), *GetName());
		}
		return true;
	}

	return false;
}

bool UActionWait::Pause(const UActionBrainComponentAction* PausedBy)
{
	bool bResult = Super::Pause(PausedBy);

	if (bResult && GetWorld()->GetTimerManager().IsTimerActive(WaitTimerHandle))
	{
		UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("%s> Paused wait timer."), *GetName());
		GetWorld()->GetTimerManager().PauseTimer(WaitTimerHandle);
	}
	
	return true;
}

bool UActionWait::Resume()
{
	bool bResult = Super::Resume();
	
	if (bResult && GetWorld()->GetTimerManager().IsTimerActive(WaitTimerHandle))
	{
		UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("%s> Resumed wait timer."), *GetName());
		GetWorld()->GetTimerManager().UnPauseTimer(WaitTimerHandle);
	}

	return true;
}

EPawnActionAbortState::Type UActionWait::PerformAbort(EAIForceParam::Type ShouldForce)
{
	if (GetWorld()->GetTimerManager().IsTimerActive(WaitTimerHandle))
	{
		UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("%s> Cleared wait timer."), *GetName());
		GetWorld()->GetTimerManager().ClearTimer(WaitTimerHandle);
	}

	return EPawnActionAbortState::AbortDone;
}

void UActionWait::WaitComplete()
{
	UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("%s> Wait completed."), *GetName());
	Finish(EPawnActionResult::Success);
}
