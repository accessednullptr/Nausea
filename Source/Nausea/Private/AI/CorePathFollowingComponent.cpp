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