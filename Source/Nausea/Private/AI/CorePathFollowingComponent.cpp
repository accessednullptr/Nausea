// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "AI/CorePathFollowingComponent.h"
#include "Navigation/CrowdManager.h"
#include "DrawDebugHelpers.h"
#include "AIConfig.h"
#include "AI/CoreAIController.h"
#include "AI/CoreAIPerceptionSystem.h"
#include "AI/CoreAIPerceptionComponent.h"
#include "AI/EnemySelectionComponent.h"
#include "AI/EnemySelection/AITargetInterface.h"
#include "GameFramework/Character.h"
#include "Character/CoreCharacterMovementComponent.h"

UCorePathFollowingComponent::UCorePathFollowingComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAffectFallingVelocity = true;	
}

void UCorePathFollowingComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (ACoreAIController* AIController = Cast<ACoreAIController>(GetOwner()))
	{
		CoreAIPerceptionComponent = Cast<UCoreAIPerceptionComponent>(AIController->GetPerceptionComponent());
		ensure(CoreAIPerceptionComponent);
	}

	DefaultCrowdSimulationState = GetCrowdSimulationState();
}

void UCorePathFollowingComponent::OnMoveBlockedBy(const FHitResult& BlockingImpact)
{
	Super::OnMoveBlockedBy(BlockingImpact);

	if (Cast<APawn>(BlockingImpact.GetActor()))
	{
		return;
	}

	if (!Path.IsValid() || GetWorld()->GetTimerManager().IsTimerActive(JumpCooldownHandle) || !CoreCharacterMovementComponent || CoreCharacterMovementComponent->IsFalling())
	{
		return;
	}
	
	GetWorld()->GetTimerManager().SetTimer(JumpCooldownHandle, JumpCooldown, false);

	const FVector TargetDirection = (GetCurrentTargetLocation() - CoreCharacterMovementComponent->GetActorFeetLocation()).GetSafeNormal2D();
	const FVector BlockedDirection = (BlockingImpact.ImpactPoint - CoreCharacterMovementComponent->GetActorFeetLocation()).GetSafeNormal2D();

	const float BlockedDot = FVector::DotProduct(TargetDirection, BlockedDirection);

	if (BlockedDot < 0.f)
	{
		return;
	}

	if (CoreCharacterMovementComponent->GetCharacterOwner()->CanJump())
	{
		CoreCharacterMovementComponent->DoJump(false);
	}
}

void UCorePathFollowingComponent::FollowPathSegment(float DeltaTime)
{
	Super::FollowPathSegment(DeltaTime);
}

void UCorePathFollowingComponent::SetMovementComponent(UNavMovementComponent* MoveComp)
{
	CoreCharacterMovementComponent = Cast<UCoreCharacterMovementComponent>(MoveComp);

	Super::SetMovementComponent(MoveComp);
}

FVector UCorePathFollowingComponent::GetMoveFocus(bool bAllowStrafe) const
{
	if (!bAllowStrafe)
	{
		return Super::GetMoveFocus(false);
	}

	FVector OwnerLocation = MovementComp->GetOwner()->GetActorLocation();
	FVector MoveFocus = FAISystem::InvalidLocation;
	ACoreAIController* CoreAIController = Cast<ACoreAIController>(GetOwner());
	UCoreAIPerceptionSystem* CoreAIPerceptionSystem = UCoreAIPerceptionSystem::GetCoreCurrent(GetOwner());
	

	auto CheckLookAtActor = [&](AActor* Target)
	{
		constexpr float CheckDistance = 2000.f * 2000.f;
		if (FVector::DistSquared(OwnerLocation, Target->GetActorLocation()) > CheckDistance)
		{
			return;
		}

		TScriptInterface<IAITargetInterface> EnemyActorInterface = Target;
		if (TSCRIPTINTERFACE_IS_VALID(EnemyActorInterface) && !UAITargetStatics::IsTargetable(EnemyActorInterface))
		{
			return;
		}

		//If target is not perceptible, ignore perception data since it would never be perceived.
		if (CoreAIPerceptionSystem && !CoreAIPerceptionSystem->DoesActorHaveStimuliSourceSenseID(Target, UCoreAIPerceptionComponent::AISenseSightID))
		{
			MoveFocus = CoreAIController->GetFocalPointOnActor(Target);
			return;
		}

		if (CoreAIPerceptionComponent)
		{
			if (CoreAIPerceptionComponent->GetStimulusAgeForActor(Target, UCoreAIPerceptionComponent::AISenseSightID) < 1.f)
			{
				MoveFocus = CoreAIController->GetFocalPointOnActor(Target);
			}
			else if (const FActorPerceptionInfo* PerceptionInfo = CoreAIPerceptionComponent->GetActorPerceptionInfo(Target))
			{
				MoveFocus = PerceptionInfo->GetStimulusLocation(UCoreAIPerceptionComponent::AISenseSightID);
			}
		}
	};

	if (AActor* Enemy = CoreAIController && CoreAIController->GetEnemySelectionComponent() ? CoreAIController->GetEnemySelectionComponent()->GetEnemy() : nullptr)
	{
		CheckLookAtActor(Enemy);

		if (MoveFocus != FAISystem::InvalidLocation)
		{
			return MoveFocus;
		}
	}

	if (DestinationActor.IsValid())
	{
		CheckLookAtActor(DestinationActor.Get());

		if (MoveFocus != FAISystem::InvalidLocation)
		{
			return MoveFocus;
		}
	}

	return Super::GetMoveFocus(false);
}

void UCorePathFollowingComponent::OnPathFinished(const FPathFollowingResult& Result)
{
	if (!Path.IsValid() || !Path->IsValid())
	{
		Super::OnPathFinished(Result);
		return;
	}

	Super::OnPathFinished(Result);
}

bool UCorePathFollowingComponent::RequestIgnoredByCrowdManager(TObjectKey<UObject> Requester)
{
	if (!Requester.ResolveObjectPtr())
	{
		return false;
	}

	IgnoredByCrowdManagerRequestList.Add(Requester);

	UpdateIgnoredByCrowdManager();
	return true;
}

bool UCorePathFollowingComponent::RevokeIgnoredByCrowdManager(TObjectKey<UObject> Requester)
{
	if (!Requester.ResolveObjectPtr())
	{
		return false;
	}

	IgnoredByCrowdManagerRequestList.Remove(Requester);
	UpdateIgnoredByCrowdManager();
	return true;
}

bool UCorePathFollowingComponent::RequestPerformPanicMovement(TObjectKey<UObject> Requester)
{
	if (!Requester.ResolveObjectPtr())
	{
		return false;
	}

	PerformPanicMovementRequestList.Add(Requester);
	UpdatePerformPanicMovement();
	return true;
}

bool UCorePathFollowingComponent::RevokePerformPanicMovement(TObjectKey<UObject> Requester)
{
	if (!Requester.ResolveObjectPtr())
	{
		return false;
	}

	PerformPanicMovementRequestList.Remove(Requester);
	UpdatePerformPanicMovement();
	return true;
}

void UCorePathFollowingComponent::UpdateIgnoredByCrowdManager()
{
	TArray<TObjectKey<UObject>> CurrentRequestList = IgnoredByCrowdManagerRequestList.Array();
	for (const TObjectKey<UObject>& Requester : CurrentRequestList)
	{
		if (!Requester.ResolveObjectPtr())
		{
			IgnoredByCrowdManagerRequestList.Remove(Requester);
		}
	}

	const bool bNewIgnoredByCrowdManager = IgnoredByCrowdManagerRequestList.Num() != 0;

	if (bIgnoredByCrowdManager == bNewIgnoredByCrowdManager)
	{
		return;
	}

	bIgnoredByCrowdManager = bNewIgnoredByCrowdManager;
	
	const ECrowdSimulationState NewSimulationState = bIgnoredByCrowdManager ? ECrowdSimulationState::Disabled : DefaultCrowdSimulationState;

	if (NewSimulationState == GetCrowdSimulationState())
	{
		return;
	}

	if (GetStatus() != EPathFollowingStatus::Idle)
	{
		AbortMove(*this, FPathFollowingResultFlags::OwnerFinished, GetCurrentRequestId(), EPathFollowingVelocityMode::Keep);
	}

	SetCrowdSimulationState(bIgnoredByCrowdManager ? ECrowdSimulationState::Disabled : DefaultCrowdSimulationState);
}

void UCorePathFollowingComponent::UpdatePerformPanicMovement()
{
	TArray<TObjectKey<UObject>> CurrentRequestList = PerformPanicMovementRequestList.Array();
	for (const TObjectKey<UObject>& Requester : CurrentRequestList)
	{
		if (!Requester.ResolveObjectPtr())
		{
			PerformPanicMovementRequestList.Remove(Requester);
		}
	}

	const bool bNewPerformPanicMovement = PerformPanicMovementRequestList.Num() != 0;

	if (bPerformPanicMovement == bNewPerformPanicMovement)
	{
		return;
	}


}