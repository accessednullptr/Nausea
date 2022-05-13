// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Vehicle/WheeledPilotVehicle.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Character/Pilot/VehicleComponent.h"
#include "Character/Pilot/VehicleSeatComponent.h"
#include "Character/Pilot/VehicleInteractableComponent.h"
#include "Gameplay/StatusComponent.h"
#include "Character/PilotCharacter.h"
#include "Gameplay/PawnInteractionComponent.h"

AWheeledPilotVehicle::AWheeledPilotVehicle(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	VehicleComponent = CreateDefaultSubobject<UVehicleComponent>(TEXT("VehicleComponent"));
	InteractableComponent = CreateDefaultSubobject<UVehicleInteractableComponent>(TEXT("VehicleInteractableComponent"));
}

void AWheeledPilotVehicle::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (StatusComponent == NULL || StatusComponent->IsPendingKill() == true)
	{
		StatusComponent = FindComponentByClass<UStatusComponent>();
	}
}

void AWheeledPilotVehicle::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);
	
	InputComponent->BindAction("Interact", EInputEvent::IE_Pressed, this, &AWheeledPilotVehicle::InteractPressed);
	InputComponent->BindAction("Interact", EInputEvent::IE_Released, this, &AWheeledPilotVehicle::InteractReleased);
}

UMeshComponent* AWheeledPilotVehicle::GetVehicleMesh() const
{
	return GetMesh();
}

FVector AWheeledPilotVehicle::CalculateRelativeSeatLocation(UVehicleSeatComponent* Seat) const
{
	return GetVehicleMesh()->GetSocketLocation(Seat->GetSeatSocketName());
}

bool AWheeledPilotVehicle::CanInteract(UPawnInteractionComponent* InstigatorComponent, const FVector& Location, const FVector& Direction) const
{
	if (!IInteractableInterface::CanInteract(InstigatorComponent, Location, Direction))
	{
		return false;
	}

	if (!VehicleComponent || !VehicleComponent->IsActive())
	{
		return false;
	}

	if (StatusComponent && StatusComponent->IsDead())
	{
		return false;
	}

	return true;
}

void AWheeledPilotVehicle::OnInteraction(UPawnInteractionComponent* InstigatorComponent, const FVector& Location, const FVector& Direction)
{
	IInteractableInterface::OnInteraction(InstigatorComponent, Location, Direction);

	if (!InstigatorComponent || !InstigatorComponent->IsLocallyOwned())
	{
		return;
	}
	
	if (APilotCharacter* PilotCharacter = InstigatorComponent->GetOwningPilotCharacter())
	{
		PilotCharacter->EnterVehicleSeat(VehicleComponent->GetBestSeatForViewpoint(PilotCharacter, Location, Direction));
	}
}

void AWheeledPilotVehicle::InteractPressed()
{

}

void AWheeledPilotVehicle::InteractReleased()
{

}