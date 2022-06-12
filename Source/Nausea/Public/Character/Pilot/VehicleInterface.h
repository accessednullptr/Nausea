// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VehicleInterface.generated.h"

class UVehicleComponent;
class UVehicleSeatComponent;

UINTERFACE()
class UVehicleInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class NAUSEA_API IVehicleInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	virtual UVehicleComponent* GetVehicleComponent() const { return IVehicleInterface::Execute_K2_GetVehicleComponent(Cast<UObject>(this)); }
	virtual FVector CalculateRelativeSeatLocation(UVehicleSeatComponent* Seat) const { return IVehicleInterface::Execute_K2_CalculateRelativeSeatLocation(Cast<UObject>(this), Seat); }

	//Implementations should cache the result of the K2 and return it in C++ (if it doesn't change).
	virtual UMeshComponent* GetVehicleMesh() const { return IVehicleInterface::Execute_K2_GetVehicleMesh(Cast<UObject>(this)); }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = VehicleInterface, meta = (DisplayName="Get Vehicle Component"))
	UVehicleComponent* K2_GetVehicleComponent() const;
	UFUNCTION(BlueprintImplementableEvent, Category = VehicleInterface, meta = (DisplayName="Calculate Relative Seat Location"))
	FVector K2_CalculateRelativeSeatLocation(UVehicleSeatComponent* Seat) const;
	UFUNCTION(BlueprintImplementableEvent, Category = VehicleInterface, meta = (DisplayName="Calculate Relative Seat Location"))
	UMeshComponent* K2_GetVehicleMesh() const;
};

UCLASS(Abstract)
class NAUSEA_API UVehicleInterfaceStatics : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = VehicleInterface)
	static UVehicleComponent* GetVehicleComponent(TScriptInterface<IVehicleInterface> Target);

	UFUNCTION(BlueprintCallable, Category = VehicleInterface)
	static TArray<UVehicleSeatComponent*> GetVehicleSeats(TScriptInterface<IVehicleInterface> Target);

	UFUNCTION(BlueprintCallable, Category = VehicleInterface)
	static UMeshComponent* GetVehicleMesh(TScriptInterface<IVehicleInterface> Target);
};