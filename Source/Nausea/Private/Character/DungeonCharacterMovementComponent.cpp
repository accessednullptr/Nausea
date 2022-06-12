// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Character/DungeonCharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "NauseaGlobalDefines.h"
#include "AI/DungeonPathFollowingComponent.h"
#include "Character/DungeonCharacter.h"
#include "Components/CapsuleComponent.h"

UDungeonCharacterMovementComponent::UDungeonCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultLandMovementMode = EMovementMode::MOVE_NavWalking;
	bProjectNavMeshWalking = true;

	bUseRVOAvoidance = true;
	AvoidanceConsiderationRadius = 32.f;
	AvoidanceWeight = 0.1f;
}

void UDungeonCharacterMovementComponent::SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode)
{
	Super::SetMovementMode(NewMovementMode, NewCustomMode);

	if (MovementMode == MOVE_Walking && GetWorld() && !GetWorld()->GetTimerManager().IsTimerActive(AttemptResetToNavMeshTimerHandle))
	{
		TWeakObjectPtr<UDungeonCharacterMovementComponent> WeakThis(this);
		GetWorld()->GetTimerManager().SetTimer(AttemptResetToNavMeshTimerHandle, FTimerDelegate::CreateWeakLambda(this, [WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				if (UDungeonPathFollowingComponent* PathFollowingComponent = Cast<UDungeonPathFollowingComponent>(WeakThis->GetPathFollowingAgent()))
				{
					PathFollowingComponent->OnLanded();
				}

				WeakThis->SetGroundMovementMode(MOVE_NavWalking);
			}
		}), 1.f, false);
	}
}

bool UDungeonCharacterMovementComponent::FindNavFloor(const FVector& TestLocation, FNavLocation& NavFloorLocation) const
{
	if (Super::FindNavFloor(TestLocation, NavFloorLocation))
	{
		return true;
	}

	return false;
}

void UDungeonCharacterMovementComponent::PerformMovement(float DeltaTime)
{
	if (!GetCharacterOwner())
	{
		return;
	}

	if (PerformUnstuck())
	{
		bSweepWhileNavWalking = false;
	}
	
	{
		const FScopedPreventAttachedComponentMove PreventMeshMovement(MovementMode == MOVE_NavWalking ? GetCharacterOwner()->GetMesh() : nullptr);
		Super::PerformMovement(DeltaTime);
	}

	//Similar concept to net correction smoothing but for nav walking.
	const FVector DefaultRelativeLocation = GetCharacterOwner()->GetClass()->GetDefaultObject<ACharacter>()->GetMesh()->GetRelativeLocation();
	const FVector CurrentRelativeLocation = GetCharacterOwner()->GetMesh()->GetRelativeLocation();
	if (!IsNetMode(NM_DedicatedServer) && !DefaultRelativeLocation.Equals(CurrentRelativeLocation))
	{
		const FVector DesiredRelativeLocation = FMath::VInterpTo(CurrentRelativeLocation, DefaultRelativeLocation, DeltaTime, 4.f);
		const FVector PulledDesiredRelativeLocation = FMath::VInterpConstantTo(DesiredRelativeLocation, DefaultRelativeLocation, DeltaTime, Velocity.Size() * 0.75f);

		if (PulledDesiredRelativeLocation.Equals(DefaultRelativeLocation, 2.f))
		{
			GetCharacterOwner()->GetMesh()->SetRelativeLocation(DefaultRelativeLocation);
		}
		else
		{
			GetCharacterOwner()->GetMesh()->SetRelativeLocation(PulledDesiredRelativeLocation);
		}
	}

	bSweepWhileNavWalking = true;
	ConsumeUnstuckTime(DeltaTime);
}

bool UDungeonCharacterMovementComponent::TryToLeaveNavWalking()
{
	SetNavWalkingPhysics(false);

	const FScopedPreventAttachedComponentMove PreventMeshMovement(GetCharacterOwner()->GetMesh());
	bool bSucceeded = true;
	if (CharacterOwner)
	{
		FVector CollisionFreeLocation = UpdatedComponent->GetComponentLocation();
		bSucceeded = GetWorld()->FindTeleportSpot(CharacterOwner, CollisionFreeLocation, UpdatedComponent->GetComponentRotation());
		if (bSucceeded)
		{
			CharacterOwner->SetActorLocation(CollisionFreeLocation);
		}
		else
		{
			FHitResult HitResult;
			FCollisionShape CollisionShape = UpdatedPrimitive->GetCollisionShape(4.f);
			FCollisionObjectQueryParams CollisionObjectQueryParams(ECC_DungeonPawn);
			if (!GetWorld()->SweepSingleByObjectType(HitResult, UpdatedPrimitive->GetComponentLocation(), UpdatedPrimitive->GetComponentLocation() + (CharacterOwner->GetActorForwardVector() * 32.f),
				UpdatedPrimitive->GetComponentRotation().Quaternion(), CollisionObjectQueryParams, CollisionShape))
			{
				bSucceeded = true;
			}
			else if(HitResult.GetComponent())
			{
				FVector ClosestPoint;
				const float Result = HitResult.GetComponent()->GetClosestPointOnCollision(UpdatedPrimitive->GetComponentLocation(), ClosestPoint);
				if (Result > 0.f)
				{
					const FVector PushDirection = (UpdatedPrimitive->GetComponentLocation() - ClosestPoint).GetSafeNormal();
					CharacterOwner->SetActorLocation(UpdatedPrimitive->GetComponentLocation() + (PushDirection * CollisionShape.Capsule.HalfHeight));
					bSucceeded = true;
				}
				else
				{
					CharacterOwner->SetActorLocation(UpdatedPrimitive->GetComponentLocation() + (FVector::UpVector * CollisionShape.Capsule.HalfHeight));
				}
			}
			
			if (!bSucceeded)
			{
				SetNavWalkingPhysics(true);
			}
		}
	}

	if (MovementMode == MOVE_NavWalking && bSucceeded)
	{
		SetMovementMode(DefaultLandMovementMode != MOVE_NavWalking ? DefaultLandMovementMode.GetValue() : MOVE_Walking);
	}
	else if (MovementMode != MOVE_NavWalking && !bSucceeded)
	{
		SetMovementMode(MOVE_NavWalking);
	}
	
	bWantsToLeaveNavWalking = !bSucceeded;
	return bSucceeded;
}