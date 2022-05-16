// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Character/Pilot/VehicleSeatComponent.h"
#include "NauseaNetDefines.h"
#include "Character/PilotCharacter.h"
#include "Character/Pilot/VehicleComponent.h"

UVehicleSeatComponent::UVehicleSeatComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);
	SetAutoActivate(true);
}

void UVehicleSeatComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(UVehicleSeatComponent, CurrentOccupant, PushReplicationParams::Default);
}

void UVehicleSeatComponent::BeginPlay()
{
	Super::BeginPlay();

}

void UVehicleSeatComponent::Activate(bool bReset)
{
	if (bReset || ShouldActivate() == true)
	{
		SetActiveFlag(true);
		OnComponentActivated.Broadcast(this, bReset);
		GetOwner()->ForceNetUpdate();
	}
}

void UVehicleSeatComponent::Deactivate()
{
	if (ShouldActivate() == false)
	{
		if (CurrentOccupant.IsValid())
		{
			CurrentOccupant->ExitVehicleSeat(this, EVehicleExitType::Force);
		}

		SetActiveFlag(false);
		OnComponentDeactivated.Broadcast(this);
		GetOwner()->ForceNetUpdate();
	}
}

void UVehicleSeatComponent::InitializeVehicleSeatComponent(UVehicleComponent* VehicleComponent)
{
	ensure(VehicleComponent);
	OwningVehicleComponent = VehicleComponent;
	RelativeSeatLocation = OwningVehicleComponent->CalculateRelativeSeatLocation(this);
}

bool UVehicleSeatComponent::CanEnter(const APilotCharacter* PilotCharacter) const
{
	if (!PilotCharacter)
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

	if (CurrentOccupant.IsValid())
	{
		return false;
	}

	if (!OwningVehicleComponent || !OwningVehicleComponent->CanEnterVehicle(PilotCharacter, this))
	{
		return false;
	}
	
	return true;
}

bool UVehicleSeatComponent::Enter(APilotCharacter* PilotCharacter)
{
	if (!CanEnter(PilotCharacter))
	{
		return false;
	}

	CurrentOccupant = PilotCharacter;
	MARK_PROPERTY_DIRTY_FROM_NAME(UVehicleSeatComponent, CurrentOccupant, this);

	CurrentOccupant->OnVehicleEnter(this);

	if (!CurrentOccupant->IsSimulatedProxy())
	{
		OnRep_CurrentOccupant();
	}

	GetOwner()->ForceNetUpdate();

	if (APawn* Pawn = (bDriverSeat && (GetOwner()->GetLocalRole() == ROLE_Authority)) ? Cast<APawn>(GetOwner()) : nullptr)
	{
		if (AController* Controller = PilotCharacter->GetController())
		{
			Controller->Possess(Pawn);
			PilotCharacter->SetOwner(Controller); //Keep ownership for RPCs.
		}
	}

	return true;
}

bool UVehicleSeatComponent::Exit(APilotCharacter* PilotCharacter, EVehicleExitType Type)
{
	ensure((CurrentOccupant.IsValid() && PilotCharacter == CurrentOccupant) || (PilotCharacter && PilotCharacter->IsSimulatedProxy()));

	//Attempt to resolve exiting occupant (CurrentOccupant can be nullptr if we're coming from the Simulated Proxy's OnRep).
	APilotCharacter* ExitingOccupant = CurrentOccupant.IsValid() ? CurrentOccupant.Get() : PilotCharacter;

	if (!ensure(ExitingOccupant))
	{
		return false;
	}

	ExitingOccupant->OnVehicleExit(this);

	CurrentOccupant = nullptr;
	MARK_PROPERTY_DIRTY_FROM_NAME(UVehicleSeatComponent, CurrentOccupant, this);

	if (!ExitingOccupant->IsSimulatedProxy())
	{
		OnRep_CurrentOccupant();
	}

	GetOwner()->ForceNetUpdate();

	if (APawn* Pawn = (bDriverSeat && (GetOwner()->GetLocalRole() == ROLE_Authority)) ? Cast<APawn>(GetOwner()) : nullptr)
	{
		if (AController* Controller = Pawn->GetController())
		{
			Controller->Possess(ExitingOccupant);
		}
	}

	return true;
}

float UVehicleSeatComponent::GetSeatDistanceToSquared(const FVector& Location) const
{
	return FVector::DistSquared(Location, GetOwner()->GetActorTransform().TransformPosition(RelativeSeatLocation));
}

void UVehicleSeatComponent::OnRep_CurrentOccupant()
{
	if (PreviousOccupant.IsValid() && PreviousOccupant->IsSimulatedProxy())
	{
		PreviousOccupant->ExitVehicleSeat(this, EVehicleExitType::Response);
	}

	if (CurrentOccupant.IsValid() && CurrentOccupant->IsSimulatedProxy())
	{
		CurrentOccupant->EnterVehicleSeat(this);
	}

	PreviousOccupant = CurrentOccupant;
}