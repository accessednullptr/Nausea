// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "AI/RoutineManager/RoutineActionPushAction.h"
#include "AI/CoreAIController.h"
#include "AI/RoutineManagerComponent.h"
#include "AI/ActionBrainComponent.h"
#include "AI/ActionBrainComponentAction.h"

#include "AI/Action/ActionBrainDataObject.h"
#include "AI/Action/ActionPerformAbility.h"
#include "Gameplay/AbilityComponent.h"

URoutineActionPushAction::URoutineActionPushAction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void URoutineActionPushAction::StartAction()
{
	if (IsActionStarted())
	{
		return;
	}

	Super::StartAction();

	ensure(GetAIController()->GetActionBrainComponent());

	CurrentAction = CreateAction();
	const bool bSuccess = GetAIController()->GetActionBrainComponent()->PerformAction(CurrentAction, Priority, this);

	if (!bSuccess)
	{
		EndAction();
	}
}

void URoutineActionPushAction::EndAction()
{
	if (IsActionCompleted())
	{
		return;
	}

	if (CurrentAction)
	{
		CurrentAction->OnActionFinished.RemoveDynamic(this, &URoutineActionPushAction::OnActionFinished);

		if (GetAIController()->GetActionBrainComponent()->AbortAction(CurrentAction) != EPawnActionAbortState::AbortDone)
		{
			const EPawnActionAbortState::Type Result = GetAIController()->GetActionBrainComponent()->ForceAbortAction(CurrentAction);
			ensure(Result == EPawnActionAbortState::AbortDone);
		}

		ensure(!GetAIController()->GetActionBrainComponent()->HasActionsForInstigator(this));
		CurrentAction = nullptr;
	}

	//Release reference.
	ActionDataObject = nullptr;

	Super::EndAction();
}

FString URoutineActionPushAction::DescribeRoutineActionToGameplayDebugger() const
{
	if (CurrentAction)
	{
		return FString::Printf(TEXT("{yellow}%s {white}pushed %s"), *GetName(), *CurrentAction->GetName());
	}

	return FString::Printf(TEXT("{yellow}%s"), *GetName());
}

UActionBrainComponentAction* URoutineActionPushAction::CreateAction()
{
	UActionBrainComponentAction* Action = UActionBrainComponentAction::CreateAction(this, ActionClass);

	if (!ensure(Action))
	{
		return nullptr;
	}

	Action->OnActionFinished.AddDynamic(this, &URoutineActionPushAction::OnActionFinished);

	if (ActionDataObject)
	{
		Action->SetDataObject(ActionDataObject);
	}

	K2_OnActionCreated(Action);
	
	return Action;
}

void URoutineActionPushAction::OnActionFinished(UActionBrainComponentAction* Action, TEnumAsByte<EPawnActionResult::Type> Result)
{
	if (Action != CurrentAction)
	{
		return;
	}

	//GetAIController()->GetActionBrainComponent()->ForceAbortAction(Action);

	if (!bLoopAction || (bEndOnActionFailure && Result != EPawnActionResult::Success))
	{	
		EndAction();
		return;
	}

	if (CurrentAction)
	{
		CurrentAction->OnActionFinished.RemoveDynamic(this, &URoutineActionPushAction::OnActionFinished);
		CurrentAction = nullptr;
	}

	CurrentAction = CreateAction();

	GetAIController()->GetActionBrainComponent()->PerformAction(CurrentAction, Priority, this);
}

URoutineActionPushAbilityAction::URoutineActionPushAbilityAction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

UActionBrainComponentAction* URoutineActionPushAbilityAction::CreateAction()
{
	if (!AbilityClass)
	{
		return nullptr;
	}

	const UAbilityInfo* AbilityInfo = AbilityClass.GetDefaultObject();

	if (!AbilityInfo)
	{
		return Super::CreateAction();
	}

	if (AbilityInfo->HasMoveActionBrainDataObject())
	{
		ActionClass = AbilityInfo->GetMoveAndPerformAbilityClass();
	}
	else if (AbilityInfo->HasTargetActionBrainDataObject())
	{
		ActionClass = AbilityInfo->GetPerformAbilityClass();
	}
	else
	{
		ActionClass = GetClass()->GetDefaultObject<URoutineActionPushAbilityAction>()->ActionClass;
	}

	UActionBrainComponentAction* Action = Super::CreateAction();
	if (IActionBrainAbilityInterface* ActionAbilityInterface = Cast<IActionBrainAbilityInterface>(Action))
	{
		ActionAbilityInterface->SetAbilityClass(AbilityClass);
		
		if (bOverrideCompleteCondition)
		{
			ActionAbilityInterface->SetOverrideCompleteCondition(OverrideCompleteCondition);
		}
	}

	return Action;
}

void URoutineActionPushAbilityAction::SetOverrideCompleteCondition(ECompleteCondition CompleteConditionOverride)
{
	OverrideCompleteCondition = CompleteConditionOverride;
	bOverrideCompleteCondition = true;
}