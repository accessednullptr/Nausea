// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Character/Pilot/VehicleTypes.h"
#include "PilotCharacter.generated.h"

class UPilotMovementComponent;
class UVehicleComponent;
class UVehicleSeatComponent;

/**
 * 
 */
UCLASS()
class NAUSEA_API APilotCharacter : public ACharacter
{
	GENERATED_UCLASS_BODY()

friend UVehicleSeatComponent;

public:
	UFUNCTION(BlueprintCallable, Category = Character)
	bool IsAutonomousProxy() const { return GetLocalRole() == ROLE_AutonomousProxy; }

	UFUNCTION(BlueprintCallable, Category = Character)
	bool IsSimulatedProxy() const { return GetLocalRole() == ROLE_SimulatedProxy; }

	UFUNCTION(BlueprintCallable, Category = Character)
	bool IsAuthority() const { return GetLocalRole() == ROLE_Authority; }

	UFUNCTION(BlueprintCallable, Category = Character)
	bool IsNonOwningAuthority() const { return IsAuthority() && !IsLocallyControlled(); }

public:
	UFUNCTION(BlueprintCallable, Category = Character)
	UPilotMovementComponent* GetPilotMovementComponent() const { return PilotMovementComponent; }

	UFUNCTION(BlueprintCallable, Category = Character)
	bool IsInVehicle() const { return CurrentVehicleSeat.IsValid(); }
	UFUNCTION(BlueprintCallable, Category = Character)
	UVehicleSeatComponent* GetCurrentVehicleSeat() const { return CurrentVehicleSeat.Get(); }

	UFUNCTION()
	virtual bool CanEnterVehicleSeat(UVehicleSeatComponent* VehicleSeatComponent) const;
	UFUNCTION()
	virtual bool EnterVehicleSeat(UVehicleSeatComponent* VehicleSeatComponent);
	UFUNCTION()
	virtual bool ExitVehicleSeat(UVehicleSeatComponent* VehicleSeatComponent, EVehicleExitType Type);

protected:
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Reliable_EnterVehicleSeat(UVehicleSeatComponent* VehicleSeatComponent);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Reliable_ExitVehicleSeat(UVehicleSeatComponent* VehicleSeatComponent);

	UFUNCTION(Client, Reliable)
	void Client_Reliable_EnterVehicleSeatFailed(UVehicleSeatComponent* VehicleSeatComponent);
	UFUNCTION(Client, Reliable)
	void Client_Reliable_ExitVehicleSuccess(UVehicleSeatComponent* VehicleSeatComponent);
	UFUNCTION(Client, Reliable)
	void Client_Reliable_EjectedFromVehicle(UVehicleSeatComponent* VehicleSeatComponent);
	
	UFUNCTION()
	virtual void OnVehicleEnter(UVehicleSeatComponent* VehicleSeatComponent);
	UFUNCTION()
	virtual void OnVehicleExit(UVehicleSeatComponent* VehicleSeatComponent);

private:
	UPROPERTY()
	UPilotMovementComponent* PilotMovementComponent = nullptr;

	UPROPERTY()
	TWeakObjectPtr<UVehicleSeatComponent> CurrentVehicleSeat = nullptr;
};
