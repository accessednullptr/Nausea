// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#include "AI/ActionBrainComponentAction.h"
#include "AI/ActionBrainComponent.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "AIController.h"
#include "AI/Action/ActionBrainDataObject.h"
#include "VisualLogger/VisualLogger.h"
#include "UObject/ReferenceChainSearch.h"

DEFINE_LOG_CATEGORY(LogActionBrainAction);

namespace
{
	FString GetActionResultName(int64 Value)
	{
		static const UEnum* Enum = StaticEnum<EPawnActionResult::Type>();
		check(Enum);
		return Enum->GetNameStringByValue(Value);
	}
}

UActionBrainComponentAction::UActionBrainComponentAction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbortState = EPawnActionAbortState::NeverStarted;
	FinishResult = EPawnActionResult::NotStarted;

	// actions start their lives paused
	bPaused = true;
	bFailedToStart = false;
	IndexOnStack = INDEX_NONE;
}

UWorld* UActionBrainComponentAction::GetWorld() const
{
	return OwnerComponent ? OwnerComponent->GetWorld() : Cast<UWorld>(GetOuter());
}

FString UActionBrainComponentAction::GetStateDescription() const
{
	static const UEnum* AbortStateEnum = StaticEnum<EPawnActionAbortState::Type>();

	if (AbortState != EPawnActionAbortState::NotBeingAborted)
	{
		return *AbortStateEnum->GetDisplayNameTextByValue(AbortState).ToString();
	}
	return IsPaused() ? TEXT("Paused") : TEXT("Active");
}

FString UActionBrainComponentAction::GetPriorityName() const
{
	static const UEnum* Enum = StaticEnum<EAIRequestPriority::Type>();
	check(Enum);
	return Enum->GetNameStringByValue(GetPriority());
}

FString UActionBrainComponentAction::GetDisplayName() const
{
	if (!GetDataObject())
	{
		return GetName();
	}

	return FString::Printf(TEXT("%s %s"), *GetName(), *GetDataObject()->GetDebugInfoString());
}

FString UActionBrainComponentAction::GetDebugInfoString(int32 Depth) const
{
	FString String = FString::ChrN(Depth * 4, ' ') + FString::Printf(TEXT("%d. %s [%s]\n"), Depth, *GetDisplayName(), *GetStateDescription());
	
	if (ChildAction)
	{
		String += ChildAction->GetDebugInfoString(Depth + 1);
	}

	return String;
}

EPawnActionAbortState::Type UActionBrainComponentAction::Abort(EAIForceParam::Type ShouldForce)
{
	// if already aborting, and this request is not Forced, just skip it
	if (AbortState != EPawnActionAbortState::NotBeingAborted && ShouldForce != EAIForceParam::Force)
	{
		if (AbortState == EPawnActionAbortState::NeverStarted)
		{
			UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("%s> Discarding Abort request since the action has never been started yet"), *GetName());
		}
		else
		{
			UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("%s> Discarding Abort request due to action being already in abort state"), *GetName());
		}

		return AbortState;
	}

	const bool bForce = ShouldForce == EAIForceParam::Force;
	EPawnActionAbortState::Type Result = EPawnActionAbortState::NotBeingAborted;
	EPawnActionAbortState::Type ChildResult = EPawnActionAbortState::AbortDone;

	SetAbortState(EPawnActionAbortState::MarkPendingAbort);

	if (ChildAction != NULL)
	{
		ChildResult = ChildAction->Abort(ShouldForce);

		if (ChildResult == EPawnActionAbortState::NotBeingAborted)
		{
			UE_VLOG(GetController(), LogActionBrainAction, Error, TEXT("%s> ChildAction %s failed to carry out proper abortion! Things might get ugly..")
				, *GetName(), *ChildAction->GetName());

			// fake proper result and hope for the best!
			ChildResult = EPawnActionAbortState::AbortDone;
		}
	}

	if (bForce)
	{
		Result = PerformAbort(ShouldForce);
		if (Result != EPawnActionAbortState::AbortDone)
		{
			UE_VLOG(GetController(), LogActionBrainAction, Error, TEXT("%s> failed to force-abort! Things might get ugly..")
				, *GetName());

			// fake proper result and hope for the best!
			Result = EPawnActionAbortState::AbortDone;
		}
	}
	else
	{
		switch (ChildResult)
		{
		case EPawnActionAbortState::MarkPendingAbort:
			// this means child is awaiting its abort, so should parent
		case EPawnActionAbortState::LatentAbortInProgress:
			// this means child is performing time-consuming abort. Parent should wait
			Result = EPawnActionAbortState::MarkPendingAbort;
			break;

		case EPawnActionAbortState::AbortDone:
			Result = IsPaused() ? EPawnActionAbortState::MarkPendingAbort : PerformAbort(ShouldForce);
			break;

		default:
			UE_VLOG(GetController(), LogActionBrainAction, Error, TEXT("%s> Unhandled Abort State!")
				, *GetName());
			Result = EPawnActionAbortState::AbortDone;
			break;
		}
	}

	SetAbortState(Result);

	return Result;
}

ACoreCharacter* UActionBrainComponentAction::GetPawn() const
{
	return OwnerComponent ? OwnerComponent->GetPawn() : nullptr;
}

AAIController* UActionBrainComponentAction::GetController() const
{
	return OwnerComponent ? OwnerComponent->GetAIOwner() : nullptr;
}

UActionBrainComponentAction* UActionBrainComponentAction::CreateAction(const UObject* WorldContextObject, TSubclassOf<UActionBrainComponentAction> ActionClass)
{
	if (!WorldContextObject || !WorldContextObject->GetWorld())
	{
		return nullptr;
	}

	//the base class doesn't do anything... so don't allow someone to create it.
	if (!ActionClass || ActionClass == UActionBrainComponentAction::StaticClass())
	{
		return nullptr;
	}

	return NewObject<UActionBrainComponentAction>(WorldContextObject->GetWorld(), ActionClass);
}

bool UActionBrainComponentAction::SetDataObject(UActionBrainDataObject* DataObject)
{
	if (bHasBeenStarted)
	{
		UE_VLOG(GetController(), LogActionBrainAction, Error, TEXT("%s> Attempting to set DataObject to %s but action has already started!"),
			*GetName(),
			DataObject ? *DataObject->GetName() : *FString("NULL"));
		return false;
	}

	ActionDataObject = DataObject;
	return true;
}

bool UActionBrainComponentAction::Activate()
{
	bool bResult = false;

	UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("%s> Activating at priority %s! First start? %s Paused? %s")
		, *GetName()
		, *GetPriorityName()
		, HasBeenStarted() ? TEXT("NO") : TEXT("YES")
		, IsPaused() ? TEXT("YES") : TEXT("NO"));

	if (HasBeenStarted() && IsPaused())
	{
		bResult = Resume();
		if (bResult == false)
		{
			UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("%s> Failed to RESUME.")
				, *GetName());
			bFailedToStart = true;
			SetFinishResult(EPawnActionResult::Failed);
			SendEvent(EPawnActionEventType::FailedToStart);
		}
		else
		{
			K2_Resume();
		}
	}
	else
	{
		bResult = Start();
		if (bResult == false)
		{
			UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("%s> Failed to START.")
				, *GetName());
			bFailedToStart = true;
			SetFinishResult(EPawnActionResult::Failed);
			SendEvent(EPawnActionEventType::FailedToStart);
		}
		else
		{
			K2_PostStart();
		}
	}

	return bResult;
}

void UActionBrainComponentAction::OnPopped()
{
	// not calling OnFinish if action haven't actually started
	if (!bFailedToStart || bAlwaysNotifyOnFinished)
	{
		OnFinished(FinishResult);
	}
}

void UActionBrainComponentAction::CleanUp()
{
	if (IsPendingKill())
	{
		return;
	}

	if (ActionDataObject && !ActionDataObject->IsPendingKill())
	{
		ActionDataObject->CleanUp();
	}

	ActionDataObject = nullptr;
	MarkPendingKill();
}

void UActionBrainComponentAction::Finish(TEnumAsByte<EPawnActionResult::Type> WithResult)
{
	UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("%s> finishing with result %s")
		, *GetName(), *GetActionResultName(WithResult));

	SetFinishResult(WithResult);

	StopWaitingForMessages();

	SendEvent(EPawnActionEventType::FinishedExecution);
}

void UActionBrainComponentAction::SendEvent(EPawnActionEventType::Type Event)
{
	if (GetOwnerComponent() && GetOwnerComponent()->IsPendingKill() == false)
	{
		// this will get communicated to parent action if needed, latently 
		GetOwnerComponent()->OnEvent(this, Event);
	}
}

void UActionBrainComponentAction::WaitForMessage(FName MessageType, FAIRequestID InRequestID)
{
	MessageHandlers.Add(FAIMessageObserver::Create(GetOwnerComponent(), MessageType, InRequestID.GetID(), FOnAIMessage::CreateUObject(this, &UActionBrainComponentAction::HandleAIMessage)));
}

void UActionBrainComponentAction::StopWaitingForMessages()
{
	MessageHandlers.Reset();
}

void UActionBrainComponentAction::SetOwnerComponent(UActionBrainComponent* Component)
{
	if (OwnerComponent != NULL && OwnerComponent != Component)
	{
		UE_VLOG(GetController(), LogActionBrainAction, Warning, TEXT("%s> UPawnAction::SetOwnerComponent called to change already set valid owner component"), *GetName());
	}

	OwnerComponent = Component;
}

void UActionBrainComponentAction::SetInstigator(UObject* const InInstigator)
{
	if (Instigator && Instigator != InInstigator)
	{
		UE_VLOG(GetController(), LogActionBrainAction, Warning, TEXT("%s> setting Instigator to %s when already has instigator set to %s")
			, *GetName(), *Instigator->GetName(), InInstigator ? *InInstigator->GetName() : TEXT("<null>"));
	}

	Instigator = InInstigator;
}

void UActionBrainComponentAction::Tick(float DeltaTime)
{
	//...
}

bool UActionBrainComponentAction::Start()
{
	if (GetDataObject())
	{
		GetDataObject()->Initialize(GetController());
		GetDataObject()->UpdateDataObject();
	}

	AbortState = EPawnActionAbortState::NotBeingAborted;
	FinishResult = EPawnActionResult::InProgress;
	bPaused = false;
	K2_PreStart();
	return true;
}

bool UActionBrainComponentAction::Pause(const UActionBrainComponentAction* PausedBy)
{
	// parent should be paused anyway
	ensure(ParentAction == NULL || ParentAction->IsPaused() == true);

	// don't pause twice, this should be guaranteed by the PawnActionsComponent
	ensure(bPaused == false);

	if (AbortState == EPawnActionAbortState::LatentAbortInProgress || AbortState == EPawnActionAbortState::AbortDone)
	{
		UE_VLOG(GetController(), LogActionBrainAction, Warning, TEXT("%s> Not pausing due to being in unpausable aborting state"), *GetName());
		return false;
	}

	if (PausedBy)
	{
		if (PausedBy == ChildAction)
		{
			UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("%s> Paused by child action %s"), *GetName(), *PausedBy->GetName());
		}
		else
		{
			UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("%s> Paused by %s"), *GetName(), *PausedBy->GetName());
		}
	}
	else
	{
		UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("%s> Paused"), *GetName());
	}

	bPaused = true;

	if (ChildAction)
	{
		ChildAction->Pause(PausedBy);
	}

	K2_Pause();

	return bPaused;
}

bool UActionBrainComponentAction::Resume()
{
	// parent should be paused anyway
	ensure(ParentAction == NULL || ParentAction->IsPaused() == true);

	// do not unpause twice
	if (bPaused == false)
	{
		return false;
	}

	ensure(ChildAction == NULL);

	if (ChildAction)
	{
		UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("%s> Resuming child, %s"), *GetName(), *ChildAction->GetName());
		ChildAction->Resume();
	}
	else
	{
		UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("%s> Resuming."), *GetName());
		bPaused = false;
	}

	return !bPaused;
}

void UActionBrainComponentAction::OnFinished(EPawnActionResult::Type WithResult)
{
	K2_Finished(WithResult);

	OnActionFinished.Broadcast(this, WithResult);
}

void UActionBrainComponentAction::OnChildFinished(UActionBrainComponentAction* Action, EPawnActionResult::Type WithResult)
{
	UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("%s> Child \'%s\' finished with result %s")
		, *GetName(), *Action->GetName(), *GetActionResultName(WithResult));

	ensure(Action->ParentAction == this);
	ensure(ChildAction == Action);
	Action->ParentAction = NULL;
	ChildAction = NULL;
}

bool UActionBrainComponentAction::PushChildAction(UActionBrainComponentAction* Action)
{
	bool bResult = false;

	if (OwnerComponent != NULL)
	{
		UE_CVLOG(ChildAction != NULL
			, GetController(), LogActionBrainAction, Log, TEXT("%s> Pushing child action %s while already having ChildAction set to %s")
			, *GetName(), *Action->GetName(), *ChildAction->GetName());

		// copy runtime data
		// note that priority and instigator will get assigned as part of PushAction.

		bResult = OwnerComponent->PushAction(Action, GetPriority(), Instigator);

		// adding a check to make sure important data has been set 
		ensure(Action->GetPriority() == GetPriority() && Action->GetInstigator() == GetInstigator());
	}

	return bResult;
}

EPawnActionAbortState::Type UActionBrainComponentAction::PerformAbort(EAIForceParam::Type ShouldForce)
{
	if (GetDataObject() && !GetDataObject()->IsReady())
	{
		GetDataObject()->AbortRequest();
	}

	return EPawnActionAbortState::AbortDone;
}

void UActionBrainComponentAction::OnPushed()
{
	IndexOnStack = 0;
	UActionBrainComponentAction* PrevAction = ParentAction;
	while (PrevAction)
	{
		++IndexOnStack;
		PrevAction = PrevAction->ParentAction;
	}

	UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("%s> Pushed with priority %s, IndexOnStack: %d, instigator %s")
		, *GetName(), *GetPriorityName(), IndexOnStack, *GetNameSafe(Instigator));
}

void UActionBrainComponentAction::SetFinishResult(EPawnActionResult::Type Result)
{
	// once return value had been set it's no longer possible to back to InProgress
	if (Result <= EPawnActionResult::InProgress)
	{
		UE_VLOG(GetController(), LogActionBrainAction, Warning, TEXT("%s> UPawnAction::SetFinishResult setting FinishResult as EPawnActionResult::InProgress or EPawnActionResult::NotStarted - should not be happening"), *GetName());
		return;
	}

	if (FinishResult != Result)
	{
		FinishResult = Result;
	}
}

void UActionBrainComponentAction::SetAbortState(EPawnActionAbortState::Type NewAbortState)
{
	// allowing only progression
	if (NewAbortState <= AbortState)
	{
		return;
	}

	AbortState = NewAbortState;
	if (AbortState == EPawnActionAbortState::AbortDone)
	{
		SendEvent(EPawnActionEventType::FinishedAborting);
	}
}