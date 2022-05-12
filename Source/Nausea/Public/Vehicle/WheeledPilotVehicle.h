// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "Character/Pilot/VehicleInterface.h"
#include "Gameplay/InteractableInterface.h"
#include "Gameplay/StatusInterface.h"
#include "WheeledPilotVehicle.generated.h"

class UVehicleComponent;
class UInteractableComponent;
class UStatusComponent;

/**
 * 
 */
UCLASS()
class NAUSEA_API AWheeledPilotVehicle : public AWheeledVehiclePawn,
	public IVehicleInterface,
	public IInteractableInterface,
	public IStatusInterface
{
	GENERATED_UCLASS_BODY()

//~ Begin AActor Interface
protected:
	virtual void PostInitializeComponents() override;
//~ End AActor Interface
	
//~ Begin APawn Interface
public:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
//~ End APawn Interface

//~ Begin IVehicleInterface Interface
public:
	virtual UVehicleComponent* GetVehicleComponent() const override { return VehicleComponent; }
	virtual UMeshComponent* GetVehicleMesh() const override;
	virtual FVector CalculateRelativeSeatLocation(UVehicleSeatComponent* Seat) const override;
//~ Begin IVehicleInterface Interface

//~ Begin IInteractableInterface Interface
public:
	virtual UInteractableComponent* GetInteractableComponent() const override { return InteractableComponent; }
	virtual bool CanInteract(UPawnInteractionComponent* InstigatorComponent, const FVector& Location, const FVector& Direction) const override;
	virtual void OnInteraction(UPawnInteractionComponent* InstigatorComponent, const FVector& Location, const FVector& Direction) override;
//~ Begin IInteractableInterface Interface

//~ Begin IStatusInterface Interface
public:
	virtual UStatusComponent* GetStatusComponent() const override { return StatusComponent; }
//~ Begin IStatusInterface Interface


protected:
	UFUNCTION()
	void InteractPressed();
	UFUNCTION()
	void InteractReleased();

private:
	UPROPERTY(EditDefaultsOnly, Category = Vehicle)
	UVehicleComponent* VehicleComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = Interaction)
	UInteractableComponent* InteractableComponent = nullptr;

	UPROPERTY(Transient)
	UStatusComponent* StatusComponent = nullptr;
};