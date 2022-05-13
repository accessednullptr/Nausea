// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "System/DebugCategory/GameplayDebuggerCategory_Vehicle.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "AITypes.h"
#include "Character/PilotCharacter.h"
#include "Character/Pilot/VehicleInterface.h"
#include "Character/Pilot/VehicleComponent.h"
#include "Character/Pilot/VehicleSeatComponent.h"

FGameplayDebuggerCategory_Vehicle::FGameplayDebuggerCategory_Vehicle()
{
	CollectDataInterval = 0.5f;
	SetDataPackReplication<FRepData>(&DataPack);
}

void FGameplayDebuggerCategory_Vehicle::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
	UVehicleComponent* VehicleComponent = UVehicleInterfaceStatics::GetVehicleComponent(DebugActor);

	DataPack.VehicleDescription = FString();
	DataPack.VehicleLocation = FAISystem::InvalidLocation;
	DataPack.VehicleRotation = FAISystem::InvalidRotation;

	if (!VehicleComponent)
	{
		return;
	}

	DataPack.VehicleLocation = DebugActor->GetActorLocation();
	DataPack.VehicleRotation = DebugActor->GetActorRotation();

	FString& Description = DataPack.VehicleDescription;
	Description.Append(FString::Printf(TEXT("Vehicle: %s (%s) {grey}(Loc: %s Rot: %s){white}\n"), *DebugActor->GetName(), VehicleComponent->IsActive() ? *FString("{green}active{white}") : *FString("{red}inactive{white}"),
		*DataPack.VehicleLocation.ToString(), *DataPack.VehicleRotation.ToString()));

	const TArray<UVehicleSeatComponent*>& VehicleSeatList = VehicleComponent->GetVehicleSeatComponentList();
	Description.Append(FString::Printf(TEXT("  Seats: %i\n"), VehicleSeatList.Num()));

	uint8 Index = 0;
	for (const UVehicleSeatComponent* Seat : VehicleSeatList)
	{
		Index++;
		Description.Append(FString::Printf(TEXT("    - %i: %s %s(%s) Occupant: %s\n"), Index, *GetNameSafe(Seat),
			Seat->IsDriverSeat() ? *FString("{grey}(Driver){white} ") : *FString(""), Seat->IsActive() ? *FString("{green}active{white}") : *FString("{red}inactive{white}"), *GetNameSafe(Seat->GetCurrentOccupant())));
	}
}

void FGameplayDebuggerCategory_Vehicle::DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext)
{
	if (DataPack.VehicleDescription.IsEmpty())
	{
		return;
	}

	CanvasContext.Printf(TEXT("%s"), *DataPack.VehicleDescription);
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_Vehicle::MakeInstance()
{
	return MakeShareable(new FGameplayDebuggerCategory_Vehicle());
}

void FGameplayDebuggerCategory_Vehicle::FRepData::Serialize(FArchive& Ar)
{
	Ar << VehicleDescription;
	Ar << VehicleLocation;
	Ar << VehicleRotation;
}
#endif // WITH_GAMEPLAY_DEBUGGER