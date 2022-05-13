// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "AI/AIControllerComponent.h"
#include "AI/CoreAIController.h"

UAIControllerComponent::UAIControllerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UAIControllerComponent::OnRegister()
{
	Super::OnRegister();

	OwningAIController = Cast<ACoreAIController>(GetOwner());

	ensure(OwningAIController);

	OwningAIController->OnPawnUpdated.AddDynamic(this, &UAIControllerComponent::OnPawnUpdated);
	
	K2_OnInitialized(OwningAIController);
}

void UAIControllerComponent::OnUnregister()
{
	Super::OnUnregister();

	if (!OwningAIController)
	{
		OwningAIController = Cast<ACoreAIController>(GetOwner());

		if (!OwningAIController)
		{
			return;
		}
	}

	OwningAIController->OnPawnUpdated.RemoveDynamic(this, &UAIControllerComponent::OnPawnUpdated);
}