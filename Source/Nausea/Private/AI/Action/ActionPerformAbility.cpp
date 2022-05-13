// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "AI/Action/ActionPerformAbility.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "AIController.h"
#include "VisualLogger/VisualLogger.h"
#include "AI/Action/ActionBrainDataObject.h"
#include "AI/Action/ActionMoveTo.h"
#include "Player/PlayerOwnershipInterface.h"
#include "Gameplay/AbilityComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UActionPerformAbility::UActionPerformAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

bool UActionPerformAbility::Start()
{
	if (!Super::Start())
	{
		return false;
	}

	const UAbilityInfo* AbilityInfo = AbilityClass.GetDefaultObject();

	if (!AbilityInfo)
	{
		return false;
	}

	AAIController* MyController = GetController();

	if (GetDataObject() && !GetDataObject()->IsReady())
	{
		bAwaitingDataObject = true;
		UE_VLOG(MyController, LogActionBrainAction, Log, TEXT("Awaiting data object ready."));
		GetDataObject()->OnActionBrainDataReady.AddDynamic(this, &UActionPerformAbility::OnActionDataObjectReady);
	}

	if (GetMoveTargetDataObject() && !GetMoveTargetDataObject()->IsReady())
	{
		bAwaitingMoveDataObject = true;
	}

	if (ACharacter* Character = Cast<ACharacter>(MyController ? MyController->GetPawn() : nullptr))
	{
		if (UCharacterMovementComponent* CharacterMovementComponent = Character->GetCharacterMovement())
		{
			if (AbilityInfo->ShouldAIStopMovement())
			{
				CharacterMovementComponent->StopActiveMovement();
			}

			if (AbilityInfo->ShouldAIWaitForLanding() && CharacterMovementComponent->IsFalling())
			{
				Character->LandedDelegate.AddDynamic(this, &UActionPerformAbility::OnLanded);
				bAwaitingLanding = true;
			}
		}
	}

	if (AbilityInfo->GetAICastStartDelay() > 0.f)
	{
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UActionPerformAbility::OnStartDelayExpired, AbilityInfo->GetAICastStartDelay());
		bAwaitingCastDelay = true;
	}

	if (IsReadyToPerformAbility())
	{
		PerformAbility();
	}

	return true;
}

bool UActionPerformAbility::Pause(const UActionBrainComponentAction* PausedBy)
{
	if (!Super::Pause(PausedBy))
	{
		return false;
	}

	if (PauseResponse != EPauseResponse::Ignore)
	{
		AbortAbility();
	}

	return true;
}

bool UActionPerformAbility::Resume()
{
	if (!Super::Resume())
	{
		return false;
	}

	if (PauseResponse == EPauseResponse::Abort)
	{
		Finish(EPawnActionResult::Failed);
		return true;
	}

	if ((!GetDataObject() || GetDataObject()->IsReady()) && PauseResponse == EPauseResponse::RestartOnResume)
	{
		if (!PerformAbility())
		{
			Finish(EPawnActionResult::Failed);
			return true;
		}
	}

	return true;
}

EPawnActionAbortState::Type UActionPerformAbility::PerformAbort(EAIForceParam::Type ShouldForce)
{
	EPawnActionAbortState::Type Result = Super::PerformAbort(ShouldForce);

	if (GetDataObject())
	{
		GetDataObject()->OnActionBrainDataReady.RemoveDynamic(this, &UActionPerformAbility::OnActionDataObjectReady);
	}

	AbortAbility();

	return Result;
}

void UActionPerformAbility::HandleAIMessage(UBrainComponent*, const FAIMessage& Message)
{
	if (!Message.HasFlag(FPathFollowingResultFlags::Blocked))
	{
		return;
	}

	if (!GetController() || !GetController()->GetPathFollowingComponent())
	{
		return;
	}

	GetController()->GetPathFollowingComponent()->AbortMove(*this, FPathFollowingResultFlags::Blocked, GetController()->GetCurrentMoveRequestID());
}

bool UActionPerformAbility::PerformMoveAction()
{
	AAIController* MyController = GetController();
	if (MyController == NULL)
	{
		return false;
	}

	//If we provide no move target, assume this ability does not want any movement and skip any attempts to perform the move.
	if (!GetMoveTargetDataObject() || !IsReadyToPerformAbility())
	{
		return true;
	}

	if (GetMoveTargetDataObject() && !GetMoveTargetDataObject()->IsReady() && !GoalActor && GoalLocation == FAISystem::InvalidLocation)
	{
		UE_VLOG(MyController, LogActionBrainAction, Log, TEXT("Awaiting data object or manually specified goal actor/goal location."));
		GetMoveTargetDataObject()->OnActionBrainDataReady.AddDynamic(this, &UActionPerformAbility::OnMoveDataObjectReady);
		return true;
	}

	if (bUsePathfinding && MyController->ShouldPostponePathUpdates())
	{
		UE_VLOG(MyController, LogActionBrainAction, Log, TEXT("Can't path right now, waiting..."));
		MyController->GetWorldTimerManager().SetTimer(TimerHandle_DeferredPerformMoveAction, this, &UActionPerformAbility::DeferredPerformMoveAction, 0.1f);
		return true;
	}

	if (RequestMove(MyController) == EPathFollowingRequestResult::RequestSuccessful && bAbortMoveOnBlocked)
	{
		WaitForMessage(UBrainComponent::AIMessage_MoveFinished, MyController->GetCurrentMoveRequestID());
	}

	return true;
}

void UActionPerformAbility::OnMoveDataObjectReady(UActionBrainDataObject* DataObject)
{
	if (!GetMoveTargetDataObject() || DataObject != GetMoveTargetDataObject())
	{
		return;
	}

	bAwaitingMoveDataObject = false;

	GoalActor = GetMoveTargetDataObject()->GetActor();
	GoalLocation = GetMoveTargetDataObject()->GetLocation();
	GetMoveTargetDataObject()->OnActionBrainDataReady.RemoveDynamic(this, &UActionPerformAbility::OnMoveDataObjectReady);

	if (IsReadyToPerformAbility())
	{
		PerformAbility();
	}
}

void UActionPerformAbility::SetAbilityClass(UClass* InAbilityClass)
{
	AbilityClass = InAbilityClass;

	if (!InAbilityClass)
	{
		return;
	}

	const UAbilityInfo* AbilityInfoCDO = InAbilityClass->GetDefaultObject<UAbilityInfo>();

	if (AbilityInfoCDO->HasTargetActionBrainDataObject())
	{
		SetDataObject(AbilityInfoCDO->CreateTargetActionBrainDataObject(this));
	}

	if (AbilityInfoCDO->HasMoveActionBrainDataObject())
	{
		MoveDataObject = AbilityInfoCDO->CreateMoveActionBrainDataObject(this);
		bAbortMoveOnBlocked = AbilityInfoCDO->ShouldStopMoveRequestOnBlock();
	}
}

void UActionPerformAbility::SetOverrideCompleteCondition(ECompleteCondition CompleteConditionOverride)
{
	bOverrideCompleteCondition = true;
	OverrideCompleteCondition = CompleteConditionOverride;
}

bool UActionPerformAbility::IsReadyToPerformAbility() const
{
	return !bAwaitingCastDelay && !bAwaitingLanding && !bAwaitingDataObject && !bAwaitingMoveDataObject;
}

bool UActionPerformAbility::PerformAbility()
{
	if (bIsPerformingAbility)
	{
		return false;
	}

	PerformMoveAction();

	if (!AbilityClass)
	{
		Finish(EPawnActionResult::Failed);
		return false;
	}

	const UAbilityInfo* AbilityInfoCDO = AbilityClass.GetDefaultObject();

	if (!AbilityInfoCDO)
	{
		Finish(EPawnActionResult::Failed);
		return false;
	}

	TArray<FAbilityTargetData> TargetDataList;

	UAbilityComponent* AbilityComponent = nullptr;
	IPlayerOwnershipInterface* PlayerOwnershipInterface = Cast<IPlayerOwnershipInterface>(GetController());
	if (PlayerOwnershipInterface)
	{
		AbilityComponent = PlayerOwnershipInterface->GetAbilityComponent();
	}

	if (!AbilityComponent || AbilityComponent->CanPerformAbility(AbilityClass) != EAbilityRequestResponse::Success)
	{
		Finish(EPawnActionResult::Failed);
		return false;
	}

	AbilityInfoCDO->GenerateTargetData(AbilityComponent, GetDataObject(), TargetDataList);

	if (AbilityComponent->PerformAbility(FAbilityInstanceData::GenerateInstanceData(AbilityClass, TargetDataList, &CurrentAbilityInstanceHandle)) != EAbilityRequestResponse::Success)
	{
		Finish(EPawnActionResult::Failed);
		return false;
	}

	if (AbilityComponent && !AbilityComponent->IsHandleValid(CurrentAbilityInstanceHandle))
	{
		Finish(EPawnActionResult::Success);
		return true;
	}
	
	TWeakObjectPtr<UActionPerformAbility> WeakThis = this;
	const ECompleteCondition CompleteCondition = bOverrideCompleteCondition ? OverrideCompleteCondition : AbilityInfoCDO->GetAICompleteCondition();
	
	switch (CompleteCondition)
	{
	case ECompleteCondition::CastComplete:
		AbilityComponent->OnAbilityInstanceStartupComplete.AddWeakLambda(this, [WeakThis](const UAbilityComponent*, FAbilityInstanceHandle InstanceHandle)
		{
			if (WeakThis.IsValid() || WeakThis->CurrentAbilityInstanceHandle == InstanceHandle)
			{
				WeakThis->Finish(EPawnActionResult::Success);
			}
		});
		break;
	case ECompleteCondition::ActivationComplete:
		AbilityComponent->OnAbilityInstanceComplete.AddWeakLambda(this, [WeakThis](const UAbilityComponent*, FAbilityInstanceHandle InstanceHandle)
		{
			if (WeakThis.IsValid() || WeakThis->CurrentAbilityInstanceHandle == InstanceHandle)
			{
				WeakThis->Finish(EPawnActionResult::Success);
			}
		});
		break;
	}

	AbilityComponent->OnAbilityInstanceInterrupted.AddWeakLambda(this, [WeakThis](const UAbilityComponent*, FAbilityInstanceHandle InstanceHandle)
	{
		if (WeakThis.IsValid() || WeakThis->CurrentAbilityInstanceHandle == InstanceHandle)
		{
			WeakThis->Finish(EPawnActionResult::Success);
		}
	});

	bIsPerformingAbility = true;
	return true;
}

void UActionPerformAbility::AbortAbility()
{
	if (!bIsPerformingAbility)
	{
		return;
	}

	bIsPerformingAbility = false;

	IPlayerOwnershipInterface* PlayerOwnershipInterface = Cast<IPlayerOwnershipInterface>(GetController());
	if (UAbilityComponent* AbilityComponent = PlayerOwnershipInterface ? PlayerOwnershipInterface->GetAbilityComponent() : nullptr)
	{
		AbilityComponent->InterruptAbility(CurrentAbilityInstanceHandle);
	}
}

void UActionPerformAbility::OnActionDataObjectReady(UActionBrainDataObject* DataObject)
{
	if (!GetDataObject() || DataObject != GetDataObject())
	{
		return;
	}

	if (IsPaused())
	{
		return;
	}

	GetDataObject()->OnActionBrainDataReady.RemoveDynamic(this, &UActionPerformAbility::OnActionDataObjectReady);
	bAwaitingDataObject = false;

	if (IsReadyToPerformAbility())
	{
		PerformAbility();
	}
}

void UActionPerformAbility::OnLanded(const FHitResult& HitResult)
{
	bAwaitingLanding = false;

	if (IsReadyToPerformAbility())
	{
		PerformAbility();
	}
}

void UActionPerformAbility::OnStartDelayExpired()
{
	bAwaitingCastDelay = false;
	
	if (IsReadyToPerformAbility())
	{
		PerformAbility();
	}
}

UActionSequencePerformAbility::UActionSequencePerformAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActionSequence.Empty(2);
	ActionSequence.Add(ObjectInitializer.CreateDefaultSubobject<UActionMoveTo>(this, TEXT("MoveToAction")));
	ActionSequence.Add(ObjectInitializer.CreateDefaultSubobject<UActionPerformAbility>(this, TEXT("PerformAbilityAction")));
}

void UActionSequencePerformAbility::SetAbilityClass(UClass* InAbilityClass)
{
	AbilityClass = InAbilityClass;

	if (!InAbilityClass)
	{
		return;
	}

	const UAbilityInfo* AbilityInfoCDO = InAbilityClass->GetDefaultObject<UAbilityInfo>();

	for (UActionBrainComponentAction* Action : ActionSequence)
	{
		if (UActionMoveTo* MoveToAction = AbilityInfoCDO->HasMoveActionBrainDataObject() ? Cast<UActionMoveTo>(Action) : nullptr)
		{
			MoveToAction->SetDataObject(AbilityInfoCDO->CreateMoveActionBrainDataObject(this));
		}

		if (UActionPerformAbility* PerformAbilityAction = AbilityInfoCDO->HasTargetActionBrainDataObject() ? Cast<UActionPerformAbility>(Action) : nullptr)
		{
			PerformAbilityAction->SetDataObject(AbilityInfoCDO->CreateTargetActionBrainDataObject(this));
		}
	}
}

void UActionSequencePerformAbility::SetOverrideCompleteCondition(ECompleteCondition CompleteConditionOverride)
{
	for (UActionBrainComponentAction* Action : ActionSequence)
	{
		if (UActionPerformAbility* PerformAbilityAction = Cast<UActionPerformAbility>(Action))
		{
			PerformAbilityAction->SetOverrideCompleteCondition(CompleteConditionOverride);
		}
	}
}