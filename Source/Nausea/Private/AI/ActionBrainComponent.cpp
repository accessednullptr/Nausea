// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "AI/ActionBrainComponent.h"
#include "AI/ActionBrainComponentAction.h"
#include "AI/CoreAIController.h"
#include "Character/CoreCharacter.h"
#include "VisualLogger/VisualLogger.h"

DEFINE_LOG_CATEGORY(LogActionBrain);

namespace
{
	FString GetEventName(int64 Value)
	{
		static const UEnum* Enum = StaticEnum<EPawnActionEventType::Type>();
		check(Enum);
		return Enum->GetNameStringByValue(Value);
	}

	FString GetPriorityName(int64 Value)
	{
		static const UEnum* Enum = StaticEnum<EAIRequestPriority::Type>();
		check(Enum);
		return Enum->GetNameStringByValue(Value);
	}

	FString GetActionSignature(UActionBrainComponentAction* Action)
	{
		if (Action == NULL)
		{
			return TEXT("NULL");
		}

		return FString::Printf(TEXT("[%s, %s]"), *Action->GetName(), *GetPriorityName(Action->GetPriority()));
	}

	struct FActionEvenSort
	{
		FORCEINLINE bool operator()(const FActionEvent& A, const FActionEvent& B) const
		{
			return A.Priority < B.Priority
				|| (A.Priority == B.Priority
					&& (A.EventType < B.EventType
						|| (A.EventType == B.EventType && A.Index < B.Index)));
		}
	};
}

FActionEvent::FActionEvent(UActionBrainComponentAction* InAction, EPawnActionEventType::Type InEventType, uint32 InIndex)
	: Action(InAction), EventType(InEventType), Index(InIndex)
{
	check(InAction);
	Priority = InAction->GetPriority();
}

UActionBrainComponent::UActionBrainComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;

	ActionStacks.AddZeroed(EAIRequestPriority::MAX);
}

void UActionBrainComponent::OnPawnUpdated(ACoreAIController* AIController, ACoreCharacter* Character)
{
	if (Character)
	{
		Pawn = Character;
		StartLogic();
	}
	else
	{
		Cleanup();
		Pawn = nullptr;
	}
}

void UActionBrainComponent::OnRegister()
{
	Super::OnRegister();

	ACoreAIController* AIController = Cast<ACoreAIController>(AIOwner);

	if (!AIController)
	{
		return;
	}

	if (!AIController->OnPawnUpdated.IsAlreadyBound(this, &UActionBrainComponent::OnPawnUpdated))
	{
		AIController->OnPawnUpdated.AddDynamic(this, &UActionBrainComponent::OnPawnUpdated);
	}

	if (AIController && AIController->GetPawn())
	{
		OnPawnUpdated(AIController, Cast<ACoreCharacter>(AIController->GetPawn()));
	}
}

void UActionBrainComponent::OnUnregister()
{
	Super::OnUnregister();

	ACoreAIController* AIController = Cast<ACoreAIController>(AIOwner);

	if (!AIController)
	{
		return;
	}

	if (AIController->OnPawnUpdated.IsAlreadyBound(this, &UActionBrainComponent::OnPawnUpdated))
	{
		AIController->OnPawnUpdated.RemoveDynamic(this, &UActionBrainComponent::OnPawnUpdated);
	}
}

void UActionBrainComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetPawn())
	{
		SetComponentTickEnabled(false);
		return;
	}

	if (ActionEvents.Num() > 1)
	{
		ActionEvents.Sort(FActionEvenSort());
	}

	if (ActionEvents.Num() > 0)
	{
		bIteratingActionEvents = true;

		UE_VLOG(AIOwner, LogActionBrain, Log, TEXT("Processing Action Events:"));
		
		int32 EventIndex = 0;
		while (ActionEvents.Num() > 0)
		{
			EventIndex++;
			FActionEvent& Event = ActionEvents[0];

			if (Event.Action == nullptr)
			{
				UE_VLOG(AIOwner, LogActionBrain, Warning, TEXT("%i> NULL action encountered during ActionEvents processing. May result in some notifies not being sent out."), EventIndex);
				ActionEvents.RemoveAt(0, 1, false);
				continue;
			}

			UE_VLOG(AIOwner, LogActionBrain, Log, TEXT("%i> %s has sent event %s"), EventIndex, *Event.Action->GetName(), *GetEventName(Event.EventType));

			switch (Event.EventType)
			{
			case EPawnActionEventType::InstantAbort:
				// can result in adding new ActionEvents (from child actions) and reallocating data in ActionEvents array
				// because of it, we need to operate on copy instead of reference to memory address
			{
				FActionEvent EventCopy(Event);
				EventCopy.Action->Abort(EAIForceParam::Force);
				ActionStacks[EventCopy.Priority].PopAction(EventCopy.Action);
			}
			break;
			case EPawnActionEventType::FinishedAborting:
			case EPawnActionEventType::FinishedExecution:
			case EPawnActionEventType::FailedToStart:
				ActionStacks[Event.Priority].PopAction(Event.Action);
				break;
			case EPawnActionEventType::Push:
				ActionStacks[Event.Priority].PushAction(Event.Action);
				break;
			default:
				break;
			}

			ActionEvents.RemoveAt(0, 1, false);
		}

		if (ActionEvents.Num() > 0)
		{
			UE_VLOG(AIOwner, LogActionBrain, Log, TEXT("Resetting ActionEvents."));
		}
		
		ActionEvents.Reset();

		bIteratingActionEvents = false;

		UpdateCurrentAction();
	}

	if (CurrentAction)
	{
		CurrentAction->TickAction(DeltaTime);
	}

	// it's possible we got new events with CurrentAction's tick
	if (!IsRunning() && ActionEvents.Num() == 0 && (CurrentAction == NULL || CurrentAction->WantsTick() == false))
	{
		SetComponentTickEnabled(false);
	}
}

void UActionBrainComponent::StartLogic()
{
	if (bIsRunning)
	{
		return;
	}

	bIsRunning = true;
	SetComponentTickEnabled(true);

	if (UActionBrainComponentAction* Action = UActionBrainComponentAction::CreateAction(this, DefaultActionClass))
	{
		PerformAction(Action, Action->GetActionPriority());
	}
}

void UActionBrainComponent::StopLogic(const FString& Reason)
{
	if (!bIsRunning)
	{
		return;
	}

	for (int32 PriorityIndex = 0; PriorityIndex < EAIRequestPriority::MAX; ++PriorityIndex)
	{
		UActionBrainComponentAction* Action = ActionStacks[PriorityIndex].GetTop();
		while (Action)
		{
			Action->Abort(EAIForceParam::Force);
			ActionStacks[PriorityIndex].PopAction(Action);
			Action = ActionStacks[PriorityIndex].GetTop();
		}
	}

	MessagesToProcess.Reset();
	bIsRunning = false;
}

bool UActionBrainComponent::IsRunning() const
{
	return bIsRunning;
}

bool UActionBrainComponent::IsPaused() const
{
	return bPaused;
}

void UActionBrainComponent::RestartLogic()
{
	if (IsRunning())
	{
		StopLogic("Restart");
	}

	StartLogic();
}

void UActionBrainComponent::Cleanup()
{
	StopLogic("Cleanup");
}

void UActionBrainComponent::PauseLogic(const FString& Reason)
{
	
}

EAILogicResuming::Type UActionBrainComponent::ResumeLogic(const FString& Reason)
{
	EAILogicResuming::Type Type = Super::ResumeLogic(Reason);

	switch (Type)
	{
	case EAILogicResuming::Type::RestartedInstead:
		RestartLogic();
		return EAILogicResuming::Type::RestartedInstead;
	}

	return Type;
}

FString UActionBrainComponent::GetDebugInfoString() const
{
	FString DebugInfo;

	for (int32 Priority = int32(EAIRequestPriority::Ultimate); Priority >= 0; Priority--)
	{
		FString PriorityName = UEnum::GetValueAsString<EAIRequestPriority::Type>(EAIRequestPriority::Type(Priority));
		PriorityName.RemoveFromStart("EAIRequestPriority::");

		if (ActionStacks[Priority].IsEmpty())
		{
			DebugInfo += FString::Printf(TEXT("\n%s actions:\n   (None)\n"), *PriorityName);
			continue;
		}

		const UActionBrainComponentAction* ParentmostAction = ActionStacks[Priority].GetTop();

		while (ParentmostAction->GetParentAction())
		{
			ParentmostAction = ParentmostAction->GetParentAction();
		}

		DebugInfo += FString::Printf(TEXT("\n%s actions: \n"), *PriorityName);
		DebugInfo += ParentmostAction->GetDebugInfoString(1);
	}

	return DebugInfo;
}

bool UActionBrainComponent::PerformAction(UActionBrainComponentAction* Action, TEnumAsByte<EAIRequestPriority::Type> Priority, UObject* Instigator)
{
	return PushAction(Action, Priority, Instigator);
}

bool UActionBrainComponent::OnEvent(UActionBrainComponentAction* Action, EPawnActionEventType::Type Event)
{
	if (!Action)
	{
		UE_VLOG(AIOwner, LogActionBrain, Warning, TEXT("Null Action in Action Event: Event %s")
			, *GetEventName(Event));
		return false;
	}

	bool bResult = false;

	const FActionEvent ActionEvent(Action, Event, ActionEventIndex++);

	if (Event != EPawnActionEventType::Invalid && ActionEvents.Find(ActionEvent) == INDEX_NONE)
	{
		ActionEvents.Add(ActionEvent);

		//Beginng ticking if this is the first event added and we have a pawn (UActionBrainComponent::StartLogic is called on pawn possession and will do this).
		if (ActionEvents.Num() == 1 && GetPawn())
		{
			SetComponentTickEnabled(true);
		}

		bResult = true;
		UE_VLOG(AIOwner, LogActionBrain, Log, TEXT("Added Action Event: Action %s Event %s")
			, *Action->GetName(), *GetEventName(Event));
	}
	else if (Event == EPawnActionEventType::Invalid)
	{
		UE_VLOG(AIOwner, LogActionBrain, Warning, TEXT("Ignoring Action Event: Action %s Event %s")
			, *Action->GetName(), *GetEventName(Event));
	}
	else
	{
		UE_VLOG(AIOwner, LogActionBrain, Warning, TEXT("Ignoring duplicate Action Event: Action %s Event %s")
			, *Action->GetName(), *GetEventName(Event));
	}

	return bResult;
}

bool UActionBrainComponent::PushAction(UActionBrainComponentAction* NewAction, EAIRequestPriority::Type Priority, UObject* Instigator)
{
	if (!NewAction)
	{
		return false;
	}

	if (NewAction->HasBeenStarted() == false || NewAction->IsFinished() == true)
	{
		NewAction->ExecutionPriority = Priority;
		NewAction->SetOwnerComponent(this);
		NewAction->SetInstigator(Instigator);
		return OnEvent(NewAction, EPawnActionEventType::Push);
	}

	return false;
}

EPawnActionAbortState::Type UActionBrainComponent::AbortAction(UActionBrainComponentAction* ActionToAbort)
{
	if (!ActionToAbort)
	{
		return EPawnActionAbortState::AbortDone;
	}

	const EPawnActionAbortState::Type AbortState = ActionToAbort->Abort(EAIForceParam::DoNotForce);
	if (AbortState == EPawnActionAbortState::NeverStarted)
	{
		// this is a special case. It's possible someone tried to abort an action that 
		// has just requested to be pushed and the push event has not been processed yet.
		// in such a case we'll look through the awaiting action events and remove a push event 
		// for given ActionToAbort
		RemoveEventsForAction(ActionToAbort);
	}
	return AbortState;
}

EPawnActionAbortState::Type UActionBrainComponent::ForceAbortAction(UActionBrainComponentAction* ActionToAbort)
{
	if (!ActionToAbort)
	{
		return EPawnActionAbortState::AbortDone;
	}

	return ActionToAbort->Abort(EAIForceParam::Force);
}

int32 UActionBrainComponent::AbortActionsInstigatedBy(UObject* const Instigator, TEnumAsByte<EAIRequestPriority::Type> Priority)
{
	uint32 AbortedActionsCount = 0;

	if (Priority == EAIRequestPriority::MAX)
	{
		// call for every regular priority 
		for (int32 PriorityIndex = 0; PriorityIndex < EAIRequestPriority::MAX; ++PriorityIndex)
		{
			AbortedActionsCount += AbortActionsInstigatedBy(Instigator, EAIRequestPriority::Type(PriorityIndex));
		}

		return AbortedActionsCount;
	}

	UE_VLOG(AIOwner, LogActionBrain, Log, TEXT("Removing Actions For Instigator: %s"), *GetNameSafe(Instigator));

	UActionBrainComponentAction* Action = ActionStacks[Priority].GetTop();
	while (Action)
	{
		if (Action->GetInstigator() == Instigator)
		{
			UE_VLOG(AIOwner, LogActionBrain, Log, TEXT("> Requesting InstantAbort of %s"), *GetActionSignature(Action));
			OnEvent(Action, EPawnActionEventType::InstantAbort);
			++AbortedActionsCount;
		}

		Action = Action->ParentAction;
	}

	ensure(!bIteratingActionEvents);

	for (int32 ActionIndex = ActionEvents.Num() - 1; ActionIndex >= 0; --ActionIndex)
	{
		const FActionEvent& Event = ActionEvents[ActionIndex];

		if (Event.Priority == Priority && Event.EventType == EPawnActionEventType::Push
			&& Event.Action && Event.Action->GetInstigator() == Instigator)
		{
			UE_VLOG(AIOwner, LogActionBrain, Log, TEXT("> Requesting InstantAbort of event %s for %s"), *GetEventName(Event.EventType), *GetActionSignature(Event.Action));
			ActionEvents.RemoveAtSwap(ActionIndex, 1, false);
			AbortedActionsCount++;
		}
	}

	return AbortedActionsCount;
}

bool UActionBrainComponent::HasActionsForInstigator(UObject* const Instigator) const
{
	for (int32 PriorityIndex = 0; PriorityIndex < EAIRequestPriority::MAX; ++PriorityIndex)
	{
		for (int32 ActionIndex = ActionEvents.Num() - 1; ActionIndex >= 0; --ActionIndex)
		{
			const FActionEvent& Event = ActionEvents[ActionIndex];

			if (Event.Priority == PriorityIndex && Event.EventType == EPawnActionEventType::Push
				&& Event.Action && Event.Action->GetInstigator() == Instigator)
			{
				return true;
			}
		}
	}

	return false;
}

void UActionBrainComponent::RemoveEventsForAction(UActionBrainComponentAction* Action)
{
	if (!Action)
	{
		return;
	}
	ensure(!bIteratingActionEvents);

	UE_VLOG(AIOwner, LogActionBrain, Log, TEXT("Removing Events For Action: %s"), *GetActionSignature(Action));
	for (int32 ActionIndex = ActionEvents.Num() - 1; ActionIndex >= 0; --ActionIndex)
	{
		if (ActionEvents[ActionIndex].Action == Action)
		{
			UE_VLOG(AIOwner, LogActionBrain, Log, TEXT("%i> Event %s removed."), ActionIndex, *GetEventName(ActionEvents[ActionIndex].EventType));
			ActionEvents.RemoveAtSwap(ActionIndex, /*Count=*/1, /*bAllowShrinking=*/false);
		}
	}
}

void UActionBrainComponent::UpdateCurrentAction()
{
	UE_VLOG(AIOwner, LogActionBrain, Log, TEXT("Picking new current actions. Old CurrentAction %s")
		, *GetActionSignature(CurrentAction));

	// find the highest priority action available
	UActionBrainComponentAction* NewCurrentAction = NULL;
	int32 Priority = EAIRequestPriority::MAX - 1;
	do
	{
		NewCurrentAction = ActionStacks[Priority].GetTop();

	} while (NewCurrentAction == NULL && --Priority >= 0);

	// if it's a new Action then enable it
	if (CurrentAction != NewCurrentAction)
	{
		UE_VLOG(AIOwner, LogActionBrain, Log, TEXT("New action: %s")
			, *GetActionSignature(NewCurrentAction));

		if (CurrentAction != NULL && CurrentAction->IsActive())
		{
			CurrentAction->Pause(NewCurrentAction);
		}
		CurrentAction = NewCurrentAction;
		bool bNewActionStartedSuccessfully = true;
		if (CurrentAction != NULL)
		{
			bNewActionStartedSuccessfully = CurrentAction->Activate();
		}

		if (bNewActionStartedSuccessfully == false)
		{
			UE_VLOG(AIOwner, LogActionBrain, Warning, TEXT("CurrentAction %s failed to activate. Removing and re-running action selection")
				, *GetActionSignature(NewCurrentAction));

			CurrentAction = NULL;
		}
	}
	else
	{
		if (CurrentAction == NULL)
		{
			UE_VLOG(AIOwner, LogActionBrain, Warning, TEXT("Doing nothing."));
		}
		else if (CurrentAction->IsFinished())
		{
			UE_VLOG(AIOwner, LogActionBrain, Warning, TEXT("Re-running same action"));
			CurrentAction->Activate();
		}
		else
		{
			UE_VLOG(AIOwner, LogActionBrain, Warning, TEXT("Still doing the same action"));
		}
	}
}