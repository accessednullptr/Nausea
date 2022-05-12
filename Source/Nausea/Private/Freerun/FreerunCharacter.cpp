// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#include "Freerun/FreerunCharacter.h"
#include "Components/InputComponent.h"
#include "Freerun/FreerunMovementComponent.h"

AFreerunCharacter::AFreerunCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UFreerunMovementComponent>(ACharacter::CharacterMovementComponentName))
{

}

void AFreerunCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	FreerunMovementComponent = Cast<UFreerunMovementComponent>(GetCharacterMovement());
}

void AFreerunCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AFreerunCharacter::StartSprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AFreerunCharacter::StopSprint);

	PlayerInputComponent->BindAction("LongJump", IE_Pressed, this, &AFreerunCharacter::PressLongJump);
}

void AFreerunCharacter::Jump()
{
	if (IsLocallyControlled() && GetFreerunMovementComponent() && GetFreerunMovementComponent()->IsJumpCooldownActive())
	{
		return;
	}

	Super::Jump();
}

bool AFreerunCharacter::CanJumpInternal_Implementation() const
{
	if (!GetCharacterMovement())
	{
		return false;
	}

	if (GetFreerunMovementComponent()->IsJumpCooldownActive())
	{
		return false;
	}

	if (!GetCharacterMovement()->IsJumpAllowed())
	{
		return false;
	}

	if (!GetCharacterMovement()->IsMovingOnGround() && !GetCharacterMovement()->IsFalling())
	{
		return false;
	}

	if (JumpCurrentCount >= JumpMaxCount)
	{
		return false;
	}

	if (!bPressedJump || JumpKeyHoldTime > GetJumpMaxHoldTime())
	{
		return false;
	}

	if (JumpCurrentCount > 0 && !CanWallJump())
	{
		return false;
	}

	return true;
}

bool AFreerunCharacter::CanWallJump(FOverlapResult* OverlapResult) const
{
	const FVector CheckLocation = GetActorTransform().TransformPosition(WallJumpRelativeOffset);

	float AdjustedWallJumpExtentSize = WallJumpExtentSize;

	if (GetLocalRole() == ROLE_Authority && !IsLocallyControlled())
	{
		AdjustedWallJumpExtentSize *= 1.1f;
	}

	const FCollisionShape CollisionShape = FCollisionShape::MakeSphere(WallJumpExtentSize);

	FCollisionQueryParams CQP = FCollisionQueryParams::DefaultQueryParam;
	CQP.TraceTag = "WallJump";
	CQP.AddIgnoredActor(this);

	if (!OverlapResult)
	{
		return GetWorld()->OverlapAnyTestByChannel(CheckLocation, FQuat::Identity, ECC_Pawn, CollisionShape, CQP);
	}

	TArray<FOverlapResult> Overlaps;
	if (!GetWorld()->OverlapMultiByChannel(Overlaps, CheckLocation, FQuat::Identity, ECC_Pawn, CollisionShape, CQP))
	{
		return false;
	}
	
	if (Overlaps.Num() > 0)
	{
		FVector Point;
		float Distance = MAX_FLT;
		for (const FOverlapResult& Overlap : Overlaps)
		{
			UPrimitiveComponent* Component = Overlap.GetComponent();
			if (!Component)
			{
				continue;
			}

			const float ClosestPointDistance = Component->GetClosestPointOnCollision(GetMovementComponent()->GetActorFeetLocation(), Point);

			if (ClosestPointDistance < 0.f)
			{
				continue;
			}

			if (ClosestPointDistance < Distance)
			{
				*OverlapResult = Overlap;
			}
		}
	}
	return true;
}

void AFreerunCharacter::StartSprint()
{
	check(GetFreerunMovementComponent());
	GetFreerunMovementComponent()->SetSprint(true);
}

void AFreerunCharacter::StopSprint()
{
	check(GetFreerunMovementComponent());
	GetFreerunMovementComponent()->SetSprint(false);
}

void AFreerunCharacter::PressLongJump()
{
	check(GetFreerunMovementComponent());
	GetFreerunMovementComponent()->DoLongJump();
}