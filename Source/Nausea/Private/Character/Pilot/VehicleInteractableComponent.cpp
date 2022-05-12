// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "Character/Pilot/VehicleInteractableComponent.h"
#include "Character/Pilot/VehicleInterface.h"
#include "Character/Pilot/VehicleComponent.h"
#include "Character/CoreCharacter.h"
#include "Gameplay/PawnInteractionComponent.h"

UVehicleInteractableComponent::UVehicleInteractableComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bHoldInteraction = true;
	HoldInteractionDuration = 1.f;
}

void UVehicleInteractableComponent::BeginPlay()
{
	VehicleInterface = GetOwner();

	Super::BeginPlay();
}