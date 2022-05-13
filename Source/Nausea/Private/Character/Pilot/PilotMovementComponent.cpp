// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Character/Pilot/PilotMovementComponent.h"


UPilotMovementComponent::UPilotMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UPilotMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);
}