// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "Character/PilotCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Character/Pilot/PilotMovementComponent.h"
#include "Character/Pilot/VehicleComponent.h"
#include "Character/Pilot/VehicleSeatComponent.h"

APilotCharacter::APilotCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPilotMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PilotMovementComponent = Cast<UPilotMovementComponent>(GetMovementComponent());
}

bool APilotCharacter::CanEnterVehicleSeat(UVehicleSeatComponent* VehicleSeatComponent) const
{
	if (IsSimulatedProxy())
	{
		return true;
	}

	if (!VehicleSeatComponent || !VehicleSeatComponent->CanEnter(this))
	{
		return false;
	}

	return true;
}

bool APilotCharacter::EnterVehicleSeat(UVehicleSeatComponent* VehicleSeatComponent)
{
	if (!CanEnterVehicleSeat(VehicleSeatComponent))
	{
		return false;
	}

	if (!ensure(VehicleSeatComponent->Enter(this)))
	{
		return false;
	}

	if (IsAutonomousProxy())
	{
		Server_Reliable_EnterVehicleSeat(VehicleSeatComponent);
	}

	return true;
}

bool APilotCharacter::ExitVehicleSeat(UVehicleSeatComponent* VehicleSeatComponent, EVehicleExitType Type)
{
	if (!CurrentVehicleSeat.IsValid())
	{
		return false;
	}
	
	if (IsAutonomousProxy() && Type != EVehicleExitType::Response)
	{
		Server_Reliable_ExitVehicleSeat(VehicleSeatComponent);
		return true;
	}

	if (!CurrentVehicleSeat->Exit(this, Type))
	{
		return false;
	}

	return true;
}

bool APilotCharacter::Server_Reliable_EnterVehicleSeat_Validate(UVehicleSeatComponent* VehicleSeatComponent)
{
	return true;
}

void APilotCharacter::Server_Reliable_EnterVehicleSeat_Implementation(UVehicleSeatComponent* VehicleSeatComponent)
{
	if (!EnterVehicleSeat(VehicleSeatComponent))
	{
		Client_Reliable_EnterVehicleSeatFailed(VehicleSeatComponent);
	}
}

bool APilotCharacter::Server_Reliable_ExitVehicleSeat_Validate(UVehicleSeatComponent* VehicleSeatComponent)
{
	return true;
}

void APilotCharacter::Server_Reliable_ExitVehicleSeat_Implementation(UVehicleSeatComponent* VehicleSeatComponent)
{
	if (ExitVehicleSeat(VehicleSeatComponent, EVehicleExitType::Request) && IsNonOwningAuthority())
	{
		Client_Reliable_ExitVehicleSuccess(VehicleSeatComponent);
	}
}

void APilotCharacter::Client_Reliable_EnterVehicleSeatFailed_Implementation(UVehicleSeatComponent* VehicleSeatComponent)
{
	ExitVehicleSeat(VehicleSeatComponent, EVehicleExitType::Response);
}

void APilotCharacter::Client_Reliable_ExitVehicleSuccess_Implementation(UVehicleSeatComponent* VehicleSeatComponent)
{
	ExitVehicleSeat(VehicleSeatComponent, EVehicleExitType::Response);
}

void APilotCharacter::Client_Reliable_EjectedFromVehicle_Implementation(UVehicleSeatComponent* VehicleSeatComponent)
{
	ExitVehicleSeat(VehicleSeatComponent, EVehicleExitType::Response);
}

void APilotCharacter::OnVehicleEnter(UVehicleSeatComponent* VehicleSeatComponent)
{
	CurrentVehicleSeat = VehicleSeatComponent;

	GetPilotMovementComponent()->SetMovementMode(MOVE_Custom, CUSTOMMOVE_Vehicle);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	AttachToComponent(VehicleSeatComponent->GetOwningVehicleComponent()->GetVehicleMeshComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, VehicleSeatComponent->GetSeatSocketName());
}

void APilotCharacter::OnVehicleExit(UVehicleSeatComponent* VehicleSeatComponent)
{
	CurrentVehicleSeat = nullptr;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	const FRotator CorrectedRotation(0.f, GetActorRotation().Yaw, 0.f);
	SetActorRotation(CorrectedRotation);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetActorEnableCollision(true);

	GetPilotMovementComponent()->SetMovementMode(MOVE_Falling);
}