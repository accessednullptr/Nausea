// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Overlord/OverlordPawnMovement.h"

UOverlordPawnMovement::UOverlordPawnMovement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetPlaneConstraintAxisSetting(EPlaneConstraintAxisSetting::Z);
	SetPlaneConstraintEnabled(true);
}

void UOverlordPawnMovement::AddInputVector(FVector WorldVector, bool bForce)
{
	if (WorldVector != FVector::ZeroVector)
	{
		Super::AddInputVector(ConstrainDirectionToPlane(WorldVector.GetSafeNormal()).GetSafeNormal() * WorldVector.Size(), bForce);
	}
	else if (bForce)
	{
		Super::AddInputVector(WorldVector, bForce);
	}
}

void UOverlordPawnMovement::ApplyControlInputToVelocity(float DeltaTime)
{
	Super::ApplyControlInputToVelocity(DeltaTime);
}