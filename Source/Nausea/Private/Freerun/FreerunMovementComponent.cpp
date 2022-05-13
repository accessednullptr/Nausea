// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Freerun/FreerunMovementComponent.h"
#include "Freerun/FreerunCharacter.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Components/PrimitiveComponent.h"

UFreerunMovementComponent::UFreerunMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UFreerunMovementComponent::BeginPlay()
{
	FreerunCharacterOwner = Cast<AFreerunCharacter>(CharacterOwner);
	
	Super::BeginPlay();
}

float UFreerunMovementComponent::GetMaxSpeed() const
{
	return GetMaxSpeedForDirection(Velocity.GetSafeNormal2D());
}

float UFreerunMovementComponent::GetMaxAcceleration() const
{
	return Super::GetMaxAcceleration();
}

bool UFreerunMovementComponent::DoJump(bool bReplayingMoves)
{
	const AFreerunCharacter* FreerunCharacter = GetFreerunCharacter();

	if (!FreerunCharacter || !FreerunCharacter->CanJump())
	{
		return false;
	}

	if (IsMovingOnGround())
	{
		if (Super::DoJump(bReplayingMoves))
		{
			if (!bReplayingMoves)
			{
				StartJumpCooldown();
			}
			return true;
		}

		return false;
	}

	FOverlapResult OverlapResult;
	if (!FreerunCharacter->CanWallJump(&OverlapResult))
	{
		return false;
	}

	UPrimitiveComponent* JumpOffComponent = OverlapResult.GetComponent();

	if (!JumpOffComponent)
	{
		return false;
	}

	if (!bReplayingMoves)
	{
		FVector JumpOffLocation;
		JumpOffComponent->GetClosestPointOnCollision(GetActorFeetLocation(), JumpOffLocation);

		
		const float JumpOffYaw = (GetActorFeetLocation() - JumpOffLocation).ToOrientationRotator().Yaw;

		//NOTE: We must cache this so that it can be sent off to the server.
		WallJumpDirection = FRotator(WallJumpPitchAngle, JumpOffYaw, 0.f).Vector();
		DrawDebugDirectionalArrow(GetWorld(), JumpOffLocation, JumpOffLocation + (WallJumpDirection * 64.f), 64.f, FColor::Red, false, 4.f, 0, 2.f);
	}

	ensure(WallJumpDirection != FVector(MAX_FLT));

	float JumpOffSpeed = JumpZVelocity * WallJumpImpulseStrength;

	if (FreerunCharacter->JumpCurrentCount > 1)
	{
		JumpOffSpeed *= FMath::Pow(WallJumpStrengthDecay, FreerunCharacter->JumpCurrentCount);
	}

	FVector JumpOffVelocity = WallJumpDirection * JumpOffSpeed;

	if (!bReplayingMoves)
	{
		StartJumpCooldown();
	}

	const float HorizontalSpeed = Velocity.Size2D();

	const float TopZSpeed = FMath::Max(JumpZVelocity * 1.1f, Velocity.Z);

	//Null velocity going into the wall.
	FTransform Transform = FTransform(WallJumpDirection.GetSafeNormal2D().ToOrientationRotator());
	FVector VelocityInWallJumpSpace = Transform.InverseTransformVector(Velocity);
	VelocityInWallJumpSpace.X = FMath::Max(VelocityInWallJumpSpace.X, 0.f);
	Velocity = Transform.TransformVector(VelocityInWallJumpSpace);

	Velocity += JumpOffVelocity;

	Velocity.Z = FMath::Min(Velocity.Z, TopZSpeed);
	Velocity.Z = FMath::Max(Velocity.Z, JumpOffSpeed * 0.33f);

	Velocity = Velocity.GetClampedToSize2D(0.f, FMath::Max(HorizontalSpeed, GetMaxPossibleSpeed()));

	SetMovementMode(MOVE_Falling);

	return true;
}

void UFreerunMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}

void UFreerunMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

	if (bWantsLongJump)
	{
		if (!CanLongJump())
		{
			bWantsLongJump = false;
			return;
		}

		const FVector LongJumpDirection = FRotator(LongJumpPitchAngle, GetFreerunCharacter()->GetActorForwardVector().ToOrientationRotator().Yaw, 0.f).Vector();
		Velocity = LongJumpDirection * JumpZVelocity * LongJumpVelocityStrength;
		SetMovementMode(MOVE_Falling);

		if (!IsClientReplayingMoves())
		{
			GetWorld()->GetTimerManager().SetTimer(LongJumpPenaltyTimer, LongJumpSpeedPenaltyDuration, false);
		}

		bWantsLongJump = false;
	}
}

void UFreerunMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
	bWantsLongJump = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
}

FNetworkPredictionData_Client* UFreerunMovementComponent::GetPredictionData_Client() const
{
	checkSlow(PawnOwner != NULL);
	checkSlow(PawnOwner->GetLocalRole() < ROLE_Authority || (PawnOwner->GetRemoteRole() == ROLE_AutonomousProxy && GetNetMode() == NM_ListenServer));
	checkSlow(GetNetMode() == NM_Client || GetNetMode() == NM_ListenServer);

	if (!ClientPredictionData)
	{
		UFreerunMovementComponent* MutableThis = const_cast<UFreerunMovementComponent*>(this);

		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_FreerunCharacter(*this);
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
	}

	return ClientPredictionData;
}

float UFreerunMovementComponent::GetMaxSpeedForDirection(const FVector& Direction) const
{
	const float BaseMaxSpeed = Super::GetMaxSpeed();

	switch (MovementMode)
	{
	case MOVE_Walking:
	case MOVE_NavWalking:
		return BaseMaxSpeed * GetSprintVelocityMultiplier(Direction);
	}

	return BaseMaxSpeed;
}

float UFreerunMovementComponent::GetMaxPossibleSpeed() const
{
	switch (MovementMode)
	{
	case MOVE_Walking:
	case MOVE_NavWalking:
		break;
	default:
		return Super::GetMaxSpeed();
	}

	return MaxWalkSpeed * GetSprintVelocityMultiplier(GetFreerunCharacter()->GetActorForwardVector(), true);
}

float UFreerunMovementComponent::GetSprintVelocityMultiplier(const FVector& Direction, bool bCalculatingMaxPossible) const
{
	check(GetFreerunCharacter());

	if (!bWantsToSprint)
	{
		return 1.f;
	}

	float SprintMultiplier = FMath::Lerp(1.f, SprintVelocityMultiplier, FMath::Max(0.f, FVector::DotProduct(GetFreerunCharacter()->GetActorForwardVector(), Direction)));

	//Replaying moves ignores this until LongJumpPenaltyTimer is made compliant with move replaying (if that's what we want).
	if (!bCalculatingMaxPossible && !IsClientReplayingMoves() && GetWorld()->GetTimerManager().IsTimerActive(LongJumpPenaltyTimer))
	{
		return 1.f;
		const float RatioRemaining = GetWorld()->GetTimerManager().GetTimerRemaining(LongJumpPenaltyTimer) / LongJumpSpeedPenaltyDuration;


		SprintMultiplier = FMath::Lerp(SprintMultiplier, 1.f, RatioRemaining);
	}

	return SprintMultiplier;
}

void UFreerunMovementComponent::SetSprint(bool bInSprint)
{
	bWantsToSprint = bInSprint;
}

void UFreerunMovementComponent::DoLongJump()
{
	if (!CanLongJump())
	{
		return;
	}
	
	bWantsLongJump = true;
}

bool UFreerunMovementComponent::CanLongJump() const
{
	if (!GetFreerunCharacter())
	{
		return false;
	}

	if (!IsMovingOnGround() || IsCrouching())
	{
		return false;
	}

	if (!IsClientReplayingMoves() && GetWorld()->GetTimerManager().IsTimerActive(LongJumpPenaltyTimer))
	{
		return false;
	}

	float SpeedRequirement = GetMaxPossibleSpeed() * 0.95f;

	//Leniency for remote clients or replays.
	if (IsClientReplayingMoves() || (GetOwnerRole() == ROLE_Authority && !GetCoreCharacter()->IsLocallyControlled()))
	{
		SpeedRequirement *= 0.9f;
	}

	if (Velocity.Size() < SpeedRequirement)
	{
		return false;
	}

	return true;
}

void UFreerunMovementComponent::StartJumpCooldown()
{
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		return;
	}

	float AdjustedJumpCooldownTime = JumpCooldownTime;

	if (GetOwnerRole() == ROLE_Authority && !GetCoreCharacter()->IsLocallyControlled())
	{
		AdjustedJumpCooldownTime *= 0.66f;
	}

	GetWorld()->GetTimerManager().SetTimer(JumpTimer, AdjustedJumpCooldownTime, false);
}

void FSavedMove_FreerunCharacter::Clear()
{
	Super::Clear();

	bSavedWantsToSprint = false;
	bSavedWantsToLongJump = false;
	SavedWallJumpDirection = FVector(MAX_FLT);
}

uint8 FSavedMove_FreerunCharacter::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	if (bSavedWantsToSprint)
	{
		Result |= FLAG_Custom_0;
	}

	if (bSavedWantsToLongJump)
	{
		Result |= FLAG_Custom_1;
	}

	return Result;
}

bool FSavedMove_FreerunCharacter::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const
{
	if (bSavedWantsToSprint != ((FSavedMove_FreerunCharacter*)& NewMove)->bSavedWantsToSprint)
	{
		return false;
	}

	if (SavedWallJumpDirection != ((FSavedMove_FreerunCharacter*)& NewMove)->SavedWallJumpDirection)
	{
		return false;
	}

	if (bSavedWantsToLongJump != ((FSavedMove_FreerunCharacter*)& NewMove)->bSavedWantsToLongJump)
	{
		return false;
	}

	return Super::CanCombineWith(NewMove, Character, MaxDelta);
}

void FSavedMove_FreerunCharacter::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

	UFreerunMovementComponent* FreerunCharacterMovement = Cast<UFreerunMovementComponent>(Character->GetCharacterMovement());
	if (FreerunCharacterMovement)
	{
		bSavedWantsToSprint = FreerunCharacterMovement->bWantsToSprint;
		bSavedWantsToLongJump = FreerunCharacterMovement->bWantsLongJump;
		SavedWallJumpDirection = FreerunCharacterMovement->WallJumpDirection;
		FreerunCharacterMovement->WallJumpDirection = FVector(MAX_FLT);
	}

	if (SavedWallJumpDirection != FVector(MAX_FLT))
	{
		bForceNoCombine = true;
	}
}

void FSavedMove_FreerunCharacter::PrepMoveFor(ACharacter* Character)
{
	Super::PrepMoveFor(Character);

	UFreerunMovementComponent* FreerunCharacterMovement = Cast<UFreerunMovementComponent>(Character->GetCharacterMovement());
	if (FreerunCharacterMovement)
	{
		FreerunCharacterMovement->bWantsToSprint = bSavedWantsToSprint;
		FreerunCharacterMovement->bWantsLongJump = bSavedWantsToLongJump;
		FreerunCharacterMovement->WallJumpDirection = SavedWallJumpDirection;
	}
}

bool FSavedMove_FreerunCharacter::IsImportantMove(const FSavedMovePtr& LastAckedMovePtr) const
{
	if (Super::IsImportantMove(LastAckedMovePtr))
	{
		return true;
	}

	return false;
}

void FSavedMove_FreerunCharacter::PostUpdate(ACharacter* Character, EPostUpdateMode PostUpdateMode)
{
	Super::PostUpdate(Character, PostUpdateMode);

	if (PostUpdateMode == PostUpdate_Replay)
	{
		if(UFreerunMovementComponent* FreerunCharacterMovement = Cast<UFreerunMovementComponent>(Character->GetCharacterMovement()))
		{
			FreerunCharacterMovement->WallJumpDirection = FVector(MAX_FLT);
		}
	}
}

FNetworkPredictionData_Client_FreerunCharacter::FNetworkPredictionData_Client_FreerunCharacter(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{

}

FSavedMovePtr FNetworkPredictionData_Client_FreerunCharacter::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_FreerunCharacter());
}