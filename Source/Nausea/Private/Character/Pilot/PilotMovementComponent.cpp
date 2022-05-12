// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "Character/Pilot/PilotMovementComponent.h"


UPilotMovementComponent::UPilotMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UPilotMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);
}