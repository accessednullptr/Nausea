// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Overlord/PlacementSystemComponent.h"


UPlacementSystemComponent::UPlacementSystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}
