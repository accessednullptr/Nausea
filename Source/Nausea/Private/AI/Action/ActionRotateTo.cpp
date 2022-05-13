// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "AI/Action/ActionRotateTo.h"
#include "AIController.h"

UActionRotateTo::UActionRotateTo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UActionRotateTo* UActionRotateTo::CreateRotateToAction(const UObject* WorldContextObject, UObject* DesiredViewTarget, FVector DesiredViewLocation, float RotationRateModifier, bool bUseLocationAsDirection)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World)
	{
		return nullptr;
	}

	UActionRotateTo* RotateToAction = UActionBrainComponentAction::CreateActionInstance<UActionRotateTo>(World);

	if (!RotateToAction)
	{
		return nullptr;
	}

	RotateToAction->RotationRate = RotationRateModifier;
	RotateToAction->SetViewTarget(DesiredViewTarget, DesiredViewLocation, bUseLocationAsDirection);
	return RotateToAction;
}

bool UActionRotateTo::Start()
{
	if (Super::Start())
	{
		UpdateFocus();
		//UE_VLOG(GetController(), LogActionBrainAction, Log, TEXT("%s> Started wait for %f seconds."), *GetName(), GetWorld()->GetTimerManager().GetTimerRemaining(WaitTimerHandle));
		return true;
	}

	return false;
}

bool UActionRotateTo::Pause(const UActionBrainComponentAction* PausedBy)
{
	if (Super::Pause(PausedBy))
	{
		ClearFocus();
		return true;
	}

	return false;
}

bool UActionRotateTo::Resume()
{
	if (Super::Resume())
	{
		UpdateFocus();
		return true;
	}


	return false;
}

EPawnActionAbortState::Type UActionRotateTo::PerformAbort(EAIForceParam::Type ShouldForce)
{
	ClearFocus();
	return EPawnActionAbortState::AbortDone;
}

void UActionRotateTo::Finish(TEnumAsByte<EPawnActionResult::Type> WithResult)
{
	ClearFocus();
	Super::Finish(WithResult);
}

FString UActionRotateTo::GetDebugInfoString(int32 Depth) const
{
	return Super::GetDebugInfoString(Depth);
}

void UActionRotateTo::SetViewTarget(UObject* InViewTarget, FVector InViewTargetLocation, bool bUseLocationAsDirection)
{
	if (InViewTarget)
	{
		ViewTarget = InViewTarget;
		ViewTargetLocation = FAISystem::InvalidLocation;
		bLocationAsDirection = false;
		return;
	}

	ViewTarget = nullptr;
	ViewTargetLocation = MoveTemp(InViewTargetLocation);
	bLocationAsDirection = bUseLocationAsDirection;
	UpdateFocus();
}

void UActionRotateTo::SetRotationRate(float InRotationRate)
{
	RotationRate = InRotationRate;
}

void UActionRotateTo::UpdateFocus()
{
	if (!GetController())
	{
		return;
	}

	if (ViewTarget.IsValid())
	{
		GetController()->SetFocus(Cast<AActor>(ViewTarget.Get()));
	}
	else
	{
		GetController()->SetFocalPoint(ViewTargetLocation);
	}
}

void UActionRotateTo::ClearFocus()
{
	if (!GetController())
	{
		return;
	}

	GetController()->ClearFocus(EAIFocusPriority::Gameplay);
}