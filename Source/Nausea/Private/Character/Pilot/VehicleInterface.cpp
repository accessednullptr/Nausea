// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Character/Pilot/VehicleInterface.h"
#include "Character/Pilot/VehicleComponent.h"
#include "Nausea.h"

UVehicleInterface::UVehicleInterface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UVehicleInterfaceStatics::UVehicleInterfaceStatics(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UVehicleComponent* UVehicleInterfaceStatics::GetVehicleComponent(TScriptInterface<IVehicleInterface> Target)
{
	return TSCRIPTINTERFACE_CALL_FUNC_RET(Target, GetVehicleComponent, K2_GetVehicleComponent, nullptr);
}

TArray<UVehicleSeatComponent*> UVehicleInterfaceStatics::GetVehicleSeats(TScriptInterface<IVehicleInterface> Target)
{
	if (UVehicleComponent* VehicleComponent = GetVehicleComponent(Target))
	{
		return VehicleComponent->GetVehicleSeatComponentList();
	}

	return TArray<UVehicleSeatComponent*>();
}

UMeshComponent* UVehicleInterfaceStatics::GetVehicleMesh(TScriptInterface<IVehicleInterface> Target)
{
	return TSCRIPTINTERFACE_CALL_FUNC_RET(Target, GetVehicleMesh, K2_GetVehicleMesh, nullptr);
}