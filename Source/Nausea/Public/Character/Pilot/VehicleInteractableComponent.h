// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gameplay/InteractableComponent.h"
#include "VehicleInteractableComponent.generated.h"

class IVehicleInterface;

/**
 * 
 */
UCLASS()
class NAUSEA_API UVehicleInteractableComponent : public UInteractableComponent
{
	GENERATED_UCLASS_BODY()

//~ Begin UActorComponent Interface
protected:
	virtual void BeginPlay() override;
//~ End UActorComponent Interface

private:
	UPROPERTY(Transient)
	TScriptInterface<IVehicleInterface> VehicleInterface = nullptr;
};
