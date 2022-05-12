// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "AI/Action/ActionMoveTo.h"
#include "TimerManager.h"
#include "Engine/World.h"
//#include "NavigationSystem/Public/NavigationPath.h"
//#include "NavigationSystem/Public/NavigationSystem.h"
#include "AIController.h"
#include "AI/ActionBrainComponent.h"
#include "AI/Action/ActionBrainDataObject.h"
#include "VisualLogger/VisualLogger.h"

UActionMoveTo::UActionMoveTo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, GoalLocation(FAISystem::InvalidLocation)
	, AcceptableRadius(30.f)
	, bFinishOnOverlap(true)
	, bUsePathfinding(true)
	, bAllowPartialPath(true)
	, bProjectGoalToNavigation(false)
	, bUpdatePathToGoal(true)
	, bAbortChildActionOnPathChange(false)
{
	bShouldPauseMovement = true;

	// force using OnFinished notify to clear observer delegates from path when action leaves the stack
	bAlwaysNotifyOnFinished = true;
}

void UActionMoveTo::BeginDestroy()
{
	ClearTimers();
	ClearPath();

	Super::BeginDestroy();
}

bool UActionMoveTo::Start()
{
	bool bResult = Super::Start();
	if (bResult)
	{
		if (GetMoveTargetDataObject() && GetMoveTargetDataObject()->IsReady())
		{
			GoalActor = GetMoveTargetDataObject()->GetActor();
			GoalLocation = GetMoveTargetDataObject()->GetLocation();
		}

		bResult = PerformMoveAction();
	}

	return bResult;
}

EPathFollowingRequestResult::Type UActionMoveTo::RequestMove(AAIController* Controller)
{
	EPathFollowingRequestResult::Type RequestResult = EPathFollowingRequestResult::Failed;

	FAIMoveRequest MoveReq;
	MoveReq.SetUsePathfinding(bUsePathfinding);
	MoveReq.SetAllowPartialPath(bAllowPartialPath);
	MoveReq.SetProjectGoalLocation(bProjectGoalToNavigation);
	MoveReq.SetNavigationFilter(FilterClass);
	MoveReq.SetAcceptanceRadius(AcceptableRadius);
	MoveReq.SetReachTestIncludesAgentRadius(bFinishOnOverlap);
	MoveReq.SetCanStrafe(bAllowStrafe);

	if (GoalActor != NULL)
	{
		const bool bAtGoal = CheckAlreadyAtGoal(Controller, GoalActor, AcceptableRadius);
		if (bUpdatePathToGoal)
		{
			MoveReq.SetGoalActor(GoalActor);
		}
		else
		{
			MoveReq.SetGoalLocation(GoalActor->GetActorLocation());
		}

		RequestResult = bAtGoal ? EPathFollowingRequestResult::AlreadyAtGoal : Controller->MoveTo(MoveReq);
	}
	else if (FAISystem::IsValidLocation(GoalLocation))
	{
		const bool bAtGoal = CheckAlreadyAtGoal(Controller, GoalLocation, AcceptableRadius);
		MoveReq.SetGoalLocation(GoalLocation);

		RequestResult = bAtGoal ? EPathFollowingRequestResult::AlreadyAtGoal : Controller->MoveTo(MoveReq);
	}
	else
	{
		UE_VLOG(Controller, LogActionBrainAction, Warning, TEXT("UActionMoveTo::Start: no valid move goal set"));
	}

	return RequestResult;
}

bool UActionMoveTo::PerformMoveAction()
{
	AAIController* MyController = GetController();
	if (MyController == NULL)
	{
		return false;
	}

	if (GetMoveTargetDataObject() && !GetMoveTargetDataObject()->IsReady() && !GoalActor && GoalLocation == FAISystem::InvalidLocation)
	{
		UE_VLOG(MyController, LogActionBrainAction, Log, TEXT("Awaiting data object or manually specified goal actor/goal location."));
		GetMoveTargetDataObject()->OnActionBrainDataReady.AddDynamic(this, &UActionMoveTo::OnMoveDataObjectReady);
		return true;
	}

	if (bUsePathfinding && MyController->ShouldPostponePathUpdates())
	{
		UE_VLOG(MyController, LogActionBrainAction, Log, TEXT("Can't path right now, waiting..."));
		MyController->GetWorldTimerManager().SetTimer(TimerHandle_DeferredPerformMoveAction, this, &UActionMoveTo::DeferredPerformMoveAction, 0.1f);
		return true;
	}

	const EPathFollowingRequestResult::Type RequestResult = RequestMove(MyController);
	bool bResult = true;

	if (RequestResult == EPathFollowingRequestResult::RequestSuccessful)
	{
		RequestID = MyController->GetCurrentMoveRequestID();
		WaitForMessage(UBrainComponent::AIMessage_MoveFinished, RequestID);
		WaitForMessage(UBrainComponent::AIMessage_RepathFailed);
	}
	else if (RequestResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		// note that this will result in latently notifying actions component about finishing
		// so it's no problem we run it from withing Start function
		Finish(EPawnActionResult::Success);
	}
	else
	{
		bResult = false;
	}

	return bResult;
}

void UActionMoveTo::DeferredPerformMoveAction()
{
	const bool bResult = PerformMoveAction();
	if (!bResult)
	{
		Finish(EPawnActionResult::Failed);
	}
}

void UActionMoveTo::OnMoveDataObjectReady(UActionBrainDataObject* DataObject)
{
	if (!GetMoveTargetDataObject() || DataObject != GetMoveTargetDataObject())
	{
		return;
	}

	GoalActor = GetMoveTargetDataObject()->GetActor();
	GoalLocation = GetMoveTargetDataObject()->GetLocation();
	GetMoveTargetDataObject()->OnActionBrainDataReady.RemoveDynamic(this, &UActionMoveTo::OnMoveDataObjectReady);

	if (IsPaused())
	{
		return;
	}

	DeferredPerformMoveAction();
}

bool UActionMoveTo::Pause(const UActionBrainComponentAction* PausedBy)
{
	if (!Super::Pause(PausedBy))
	{
		return false;
	}

	if (RequestID.IsValid())
	{
		AAIController* MyController = Cast<AAIController>(GetController());
		if (MyController)
		{
			return MyController->PauseMove(RequestID);
		}
	}
	
	return true;
}

bool UActionMoveTo::Resume()
{
	if (GoalActor != NULL && GoalActor->IsPendingKillPending())
	{
		return false;
	}

	if (!Super::Resume())
	{
		return false;
	}

	if (!RequestID.IsValid())
	{
		if (GetMoveTargetDataObject() && !GetMoveTargetDataObject()->IsReady())
		{
			return true;
		}

		DeferredPerformMoveAction();
		return true;
	}
	
	if (AAIController* MyController = Cast<AAIController>(GetController()))
	{
		if (MyController->ResumeMove(RequestID) == false)
		{
			// try requesting a new move
			UE_VLOG(MyController, LogActionBrainAction, Log, TEXT("Resume move failed, requesting a new one."));
			StopWaitingForMessages();

			return PerformMoveAction();
		}
		else
		{
			return true;
		}
	}

	return true;
}

EPawnActionAbortState::Type UActionMoveTo::PerformAbort(EAIForceParam::Type ShouldForce)
{
	ClearTimers();
	ClearPath();

	if (GetMoveTargetDataObject())
	{
		GetMoveTargetDataObject()->OnActionBrainDataReady.RemoveDynamic(this, &UActionMoveTo::OnMoveDataObjectReady);
	}

	AAIController* MyController = Cast<AAIController>(GetController());

	if (MyController && MyController->GetPathFollowingComponent() && RequestID.IsValid())
	{
		MyController->GetPathFollowingComponent()->AbortMove(*this, FPathFollowingResultFlags::OwnerFinished, RequestID);
	}

	return Super::PerformAbort(ShouldForce);
}

void UActionMoveTo::HandleAIMessage(UBrainComponent*, const FAIMessage& Message)
{
	if (Message.MessageName == UBrainComponent::AIMessage_MoveFinished && Message.HasFlag(FPathFollowingResultFlags::NewRequest))
	{
		// move was aborted by another request from different action, don't finish yet
		return;
	}

	const bool bFail = Message.MessageName == UBrainComponent::AIMessage_RepathFailed
		|| Message.Status == FAIMessage::Failure;

	Finish(bFail ? EPawnActionResult::Failed : EPawnActionResult::Success);
}

void UActionMoveTo::OnFinished(EPawnActionResult::Type WithResult)
{
	ClearTimers();
	ClearPath();

	Super::OnFinished(WithResult);
}

void UActionMoveTo::ClearPath()
{
	ClearPendingRepath();
	if (Path.IsValid())
	{
		Path->RemoveObserver(PathObserverDelegateHandle);
		Path = NULL;
	}
}

void UActionMoveTo::SetPath(FNavPathSharedRef InPath)
{
	if (InPath != Path)
	{
		ClearPath();
		Path = InPath;
		PathObserverDelegateHandle = Path->AddObserver(FNavigationPath::FPathObserverDelegate::FDelegate::CreateUObject(this, &UActionMoveTo::OnPathUpdated));

		// skip auto updates, it will be handled manually to include controller's ShouldPostponePathUpdates()
		Path->EnableRecalculationOnInvalidation(false);
	}
}

void UActionMoveTo::OnPathUpdated(FNavigationPath* UpdatedPath, ENavPathEvent::Type Event)
{
	const AController* MyOwner = GetController();
	if (MyOwner == NULL)
	{
		return;
	}

	UE_VLOG(MyOwner, LogActionBrainAction, Log, TEXT("%s> Path updated!"), *GetName());

	if (bAbortChildActionOnPathChange && GetChildAction())
	{
		UE_VLOG(MyOwner, LogActionBrainAction, Log, TEXT(">> aborting child action: %s"), *GetNameSafe(GetChildAction()));

		GetOwnerComponent()->AbortAction(GetChildAction());
	}

	if (Event == ENavPathEvent::Invalidated)
	{
		TryToRepath();
	}

	// log new path when action is paused, otherwise it will be logged by path following component's update
	if (Event == ENavPathEvent::UpdatedDueToGoalMoved || Event == ENavPathEvent::UpdatedDueToNavigationChanged)
	{
		bool bShouldLog = UpdatedPath && IsPaused();

		// make sure it's still satisfying partial path condition
		if (UpdatedPath && UpdatedPath->IsPartial())
		{
			const bool bIsAllowed = IsPartialPathAllowed();
			if (!bIsAllowed)
			{
				UE_VLOG(MyOwner, LogActionBrainAction, Log, TEXT(">> partial path is not allowed, aborting"));
				GetOwnerComponent()->AbortAction(this);
				bShouldLog = true;
			}
		}

#if ENABLE_VISUAL_LOG
		if (bShouldLog)
		{
			UPathFollowingComponent::LogPathHelper(MyOwner, UpdatedPath, UpdatedPath->GetGoalActor());
		}
#endif
	}
}

UActionMoveTo* UActionMoveTo::CreateMoveToAction(const UObject* WorldContextObject, AActor* InGoalActor, FVector InGoalLocation, bool bInUsePathFinding)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World)
	{
		return nullptr;
	}

	UActionMoveTo* MoveToAction = UActionBrainComponentAction::CreateActionInstance<UActionMoveTo>(World);

	if (!MoveToAction)
	{
		return nullptr;
	}

	if (InGoalActor)
	{
		MoveToAction->GoalActor = InGoalActor;
	}
	else
	{
		MoveToAction->GoalLocation = InGoalLocation;
	}

	MoveToAction->bUsePathfinding = bInUsePathFinding;

	return MoveToAction;
}

void UActionMoveTo::TryToRepath()
{
	if (Path.IsValid())
	{
		AAIController* MyController = Cast<AAIController>(GetController());
		if (MyController == NULL || !MyController->ShouldPostponePathUpdates())
		{
			ANavigationData* NavData = Path->GetNavigationDataUsed();
			if (NavData)
			{
				NavData->RequestRePath(Path, ENavPathUpdateType::NavigationChanged);
			}
		}
		else if (GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(TimerHandle_TryToRepath, this, &UActionMoveTo::TryToRepath, 0.25f);
		}
	}
}

void UActionMoveTo::ClearPendingRepath()
{
	if (TimerHandle_TryToRepath.IsValid())
	{
		UWorld* World = GetWorld();
		if (World)
		{
			World->GetTimerManager().ClearTimer(TimerHandle_TryToRepath);
			TimerHandle_TryToRepath.Invalidate();
		}
	}
}

void UActionMoveTo::ClearTimers()
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(TimerHandle_DeferredPerformMoveAction);
		World->GetTimerManager().ClearTimer(TimerHandle_TryToRepath);

		TimerHandle_DeferredPerformMoveAction.Invalidate();
		TimerHandle_TryToRepath.Invalidate();
	}
}

bool UActionMoveTo::CheckAlreadyAtGoal(AAIController* Controller, const FVector& TestLocation, float Radius)
{
	const bool bAlreadyAtGoal = Controller->GetPathFollowingComponent()->HasReached(TestLocation, EPathFollowingReachMode::OverlapAgentAndGoal, Radius);
	if (bAlreadyAtGoal)
	{
		UE_VLOG(Controller, LogActionBrainAction, Log, TEXT("New move request already at goal, aborting active movement"));
		Controller->GetPathFollowingComponent()->RequestMoveWithImmediateFinish(EPathFollowingResult::Success);
	}

	return bAlreadyAtGoal;
}

bool UActionMoveTo::CheckAlreadyAtGoal(AAIController* Controller, const AActor* TestGoal, float Radius)
{
	if (!TestGoal)
	{
		return true;
	}

	const bool bAlreadyAtGoal = Controller->GetPathFollowingComponent()->HasReached(*TestGoal, EPathFollowingReachMode::OverlapAgentAndGoal, Radius);
	if (bAlreadyAtGoal)
	{
		UE_VLOG(Controller, LogActionBrainAction, Log, TEXT("New move request already at goal, aborting active movement"));
		Controller->GetPathFollowingComponent()->RequestMoveWithImmediateFinish(EPathFollowingResult::Success);
	}

	return bAlreadyAtGoal;
}

bool UActionMoveTo::IsPartialPathAllowed() const
{
	return bAllowPartialPath;
}

