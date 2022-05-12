// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Character/Pilot/VehicleTypes.h"
#include "VehicleComponent.generated.h"

class APilotCharacter;
class UVehicleSeatComponent;
class UMeshComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), HideCategories = (Variable, Sockets, Tags, ComponentTick, ComponentReplication, Activation, Cooking, Events, AssetUserData, UserAssetData, Collision))
class NAUSEA_API UVehicleComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

friend UVehicleSeatComponent;

//~ Begin UActorComponent Interface
protected:
	virtual void BeginPlay() override;
public:
	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;
//~ End UActorComponent Interface
	
public:
	UFUNCTION(BlueprintCallable, Category = VehicleComponent)
	TScriptInterface<IVehicleInterface> GetOwnerInterface() const { return VehicleInterface; }

	UFUNCTION(BlueprintCallable, Category = VehicleComponent)
	const TArray<UVehicleSeatComponent*>& GetVehicleSeatComponentList() const { return VehicleSeatComponentList; }

	UFUNCTION(BlueprintCallable, Category = VehicleComponent)
	UMeshComponent* GetVehicleMeshComponent() const { return VehicleMeshComponent; }

	UFUNCTION()
	UVehicleSeatComponent* GetBestSeatForCharacter(const APilotCharacter* PilotCharacter) const;

	UFUNCTION()
	UVehicleSeatComponent* GetBestSeatForViewpoint(const APilotCharacter* PilotCharacter, const FVector& Location, const FVector& Direction) const;

	UFUNCTION()
	FVector CalculateRelativeSeatLocation(UVehicleSeatComponent* Seat) const;

protected:
	//This function is responsible for checking if this vehicle is able to accept a given player (vehicle is not destroyed, correct team for player, etc.).
	//APilotCharacter will perform its own set of checks to see if it's able to enter vehicles before calling this and so NEVER call APilotCharacter::CanEnterVehicleSeat().
	UFUNCTION()
	virtual bool CanEnterVehicle(const APilotCharacter* PilotCharacter, const UVehicleSeatComponent* VehicleSeatComponent) const;
	
	UFUNCTION()
	void InitializeVehicleComponent();

protected:
	UPROPERTY()
	TArray<UVehicleSeatComponent*> VehicleSeatComponentList = TArray<UVehicleSeatComponent*>();

private:
	UPROPERTY(Transient)
	TScriptInterface<IVehicleInterface> VehicleInterface = nullptr;

	UPROPERTY(Transient)
	UMeshComponent* VehicleMeshComponent = nullptr;
};
