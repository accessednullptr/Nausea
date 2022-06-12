// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "AI/DungeonPathFollowingComponent.h"
#include "Character/DungeonCharacter.h"
#include "Character/DungeonCharacterMovementComponent.h"

UDungeonPathFollowingComponent::UDungeonPathFollowingComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

void UDungeonPathFollowingComponent::BeginPlay()
{
	SetCrowdSimulationState(ECrowdSimulationState::Enabled);
	SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::High);
	SetCrowdAnticipateTurns(true, true);
	SetBlockDetection(50.f, 0.1f, 5);

	Super::BeginPlay();
}

void UDungeonPathFollowingComponent::OnLanded()
{
	Super::OnLanded();

	if (Path.IsValid() && Path->IsValid())
	{
		const bool bIsRecalculatingOnInvalidation = Path->WillRecalculateOnInvalidation();

		Path->SetIgnoreInvalidation(false);
		Path->EnableRecalculationOnInvalidation(true);
		Path->Invalidate();
		Path->EnableRecalculationOnInvalidation(bIsRecalculatingOnInvalidation);
	}
}

void UDungeonPathFollowingComponent::OnPathFinished(const FPathFollowingResult& Result)
{
	if (Result.HasFlag(FPathFollowingResultFlags::Blocked) && MovementComp)
	{
		//Attempt to refresh this path and let character movement handle this.
		if (UDungeonCharacterMovementComponent* DungeonMovementComp = Cast<UDungeonCharacterMovementComponent>(MovementComp))
		{
			//We are really slow... so we're probably not blocked.
			if (DungeonMovementComp->GetMaxSpeed() > 10.f)
			{
				const bool bIsRecalculatingOnInvalidation = Path->WillRecalculateOnInvalidation();

				Path->SetIgnoreInvalidation(false);
				Path->EnableRecalculationOnInvalidation(true);
				Path->Invalidate();
				Path->EnableRecalculationOnInvalidation(bIsRecalculatingOnInvalidation);

				DungeonMovementComp->RefreshUnstuckTime();
			}


			return;
		}
	}

	if (Result.Code == EPathFollowingResult::Invalid)
	{
		OnPathFailure();
	}

	Super::OnPathFinished(Result);
}

void UDungeonPathFollowingComponent::OnPathFailure()
{
	PathFailureCount++;

	if (PathFailureCount < 20)
	{
		TWeakObjectPtr<UDungeonPathFollowingComponent> WeakThis(this);
		GetWorld()->GetTimerManager().SetTimer(PathFailureTimer, FTimerDelegate::CreateWeakLambda(this, [WeakThis]() {
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->DecrementPathFailureCounter();
			}), 1.f, false);
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(PathFailureTimer);
	PathFailureCount = 0;

	if (ADungeonCharacter* DungeonCharacter = MovementComp ? Cast<ADungeonCharacter>(MovementComp->GetOwner()) : nullptr)
	{
		DungeonCharacter->NotifyCharacterUnableToPath();
	}
}

void UDungeonPathFollowingComponent::DecrementPathFailureCounter()
{
	GetWorld()->GetTimerManager().ClearTimer(PathFailureTimer);
	PathFailureCount--;
	if (PathFailureCount <= 0)
	{
		return;
	}

	TWeakObjectPtr<UDungeonPathFollowingComponent> WeakThis(this);
	GetWorld()->GetTimerManager().SetTimer(PathFailureTimer, FTimerDelegate::CreateWeakLambda(this, [WeakThis](){
	if (!WeakThis.IsValid())
	{
		return;
	}

	WeakThis->DecrementPathFailureCounter();
	}), 1.f, false);
}