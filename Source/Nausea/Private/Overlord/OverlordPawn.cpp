// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Overlord/OverlordPawn.h"
#include "Overlord/OverlordPawnMovement.h"
#include "Components/InputComponent.h"

AOverlordPawn::AOverlordPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UOverlordPawnMovement>(ADefaultPawn::MovementComponentName))
{

}


void AOverlordPawn::SetupPlayerInputComponent(UInputComponent* InInputComponent)
{
	check(InInputComponent);

	InInputComponent->BindAxis("MoveForward", this, &ADefaultPawn::MoveForward);
	InInputComponent->BindAxis("MoveRight", this, &ADefaultPawn::MoveRight);

	InInputComponent->BindAxis("Turn", this, &AOverlordPawn::MouseLookYaw);
	InInputComponent->BindAxis("TurnRate", this, &ADefaultPawn::TurnAtRate);

	InInputComponent->BindAxis("LookUp", this, &AOverlordPawn::MouseLookPitch);
	InInputComponent->BindAxis("LookUpRate", this, &ADefaultPawn::LookUpAtRate);

	InInputComponent->BindAction("RotateCamera", IE_Pressed, this, &AOverlordPawn::RequestMouseLook);
	InInputComponent->BindAction("RotateCamera", IE_Released, this, &AOverlordPawn::ReleaseMouseLook);
}

void AOverlordPawn::MouseLookPitch(float Val)
{
	if (!bMouseLook)
	{
		return;
	}

	AddControllerPitchInput(Val);
}

void AOverlordPawn::MouseLookYaw(float Val)
{
	if (!bMouseLook)
	{
		return;
	}

	AddControllerYawInput(Val);
}

void AOverlordPawn::RequestMouseLook()
{
	bMouseLook = true;
}

void AOverlordPawn::ReleaseMouseLook()
{
	bMouseLook = false;
}