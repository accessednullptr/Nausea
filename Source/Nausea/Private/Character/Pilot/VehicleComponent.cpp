// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "Character/Pilot/VehicleComponent.h"
#include "Character/Pilot/VehicleInterface.h"
#include "Character/PilotCharacter.h"
#include "Character/Pilot/VehicleSeatComponent.h"

UVehicleComponent::UVehicleComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);
	SetAutoActivate(true);
}

void UVehicleComponent::BeginPlay()
{
	InitializeVehicleComponent();

	Super::BeginPlay();
}

void UVehicleComponent::Activate(bool bReset)
{
	if (bReset || ShouldActivate() == true)
	{
		for(UVehicleSeatComponent* VehicleSeatComponent : VehicleSeatComponentList)
		{
			if (!VehicleSeatComponent)
			{
				continue;
			}

			VehicleSeatComponent->Deactivate();
		}

		SetActiveFlag(true);
		OnComponentActivated.Broadcast(this, bReset);
		GetOwner()->ForceNetUpdate();
	}
}

void UVehicleComponent::Deactivate()
{
	if (ShouldActivate() == false)
	{
		SetActiveFlag(false);

		OnComponentDeactivated.Broadcast(this);
		GetOwner()->ForceNetUpdate();
	}
}

UVehicleSeatComponent* UVehicleComponent::GetBestSeatForCharacter(const APilotCharacter* PilotCharacter) const
{
	if (!PilotCharacter)
	{
		return nullptr;
	}

	return GetBestSeatForViewpoint(PilotCharacter, PilotCharacter->GetPawnViewLocation(), PilotCharacter->GetBaseAimRotation().Vector());
}

UVehicleSeatComponent* UVehicleComponent::GetBestSeatForViewpoint(const APilotCharacter* PilotCharacter, const FVector& Location, const FVector& Direction) const
{
	UVehicleSeatComponent* BestSeat = nullptr;
	float BestSeatDistanceSq = MAX_FLT;
	for (UVehicleSeatComponent* Seat : VehicleSeatComponentList)
	{
		const float DistanceToSq = Seat->GetSeatDistanceToSquared(Location);
		if (BestSeatDistanceSq > DistanceToSq && PilotCharacter->CanEnterVehicleSeat(Seat))
		{
			BestSeatDistanceSq = DistanceToSq;
			BestSeat = Seat;
		}
	}

	return BestSeat;
}

FVector UVehicleComponent::CalculateRelativeSeatLocation(UVehicleSeatComponent* Seat) const
{
	return TSCRIPTINTERFACE_CALL_FUNC_RET(VehicleInterface, CalculateRelativeSeatLocation, K2_CalculateRelativeSeatLocation, FAISystem::InvalidLocation, Seat);
}

bool UVehicleComponent::CanEnterVehicle(const APilotCharacter* PilotCharacter, const UVehicleSeatComponent* VehicleSeatComponent) const
{
	if (!PilotCharacter || !VehicleSeatComponent)
	{
		return false;
	}

	if (PilotCharacter->IsSimulatedProxy())
	{
		return true;
	}

	if (!IsActive())
	{
		return false;
	}

	return true;
}

void UVehicleComponent::InitializeVehicleComponent()
{
	VehicleInterface = GetOwner();

	TInlineComponentArray<UVehicleSeatComponent*> VehicleSeats(GetOwner());

	for (UVehicleSeatComponent* VehicleSeat : VehicleSeats)
	{
		if (!VehicleSeat)
		{
			continue;
		}

		VehicleSeat->InitializeVehicleSeatComponent(this);
	}

	VehicleSeatComponentList = VehicleSeats;

	VehicleMeshComponent = UVehicleInterfaceStatics::GetVehicleMesh(GetOwnerInterface());
}