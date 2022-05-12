// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AITypes.h"
#include "Character/Pilot/VehicleTypes.h"
#include "VehicleSeatComponent.generated.h"

class APilotCharacter;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), HideCategories = (Variable, Sockets, Tags, ComponentTick, ComponentReplication, Activation, Cooking, Events, AssetUserData, UserAssetData, Collision))
class NAUSEA_API UVehicleSeatComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()
		
//~ Begin UActorComponent Interface
protected:
	virtual void BeginPlay() override;
public:
	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;
//~ End UActorComponent Interface

public:
	UFUNCTION()
	virtual void InitializeVehicleSeatComponent(UVehicleComponent* VehicleComponent);

	UFUNCTION()
	UVehicleComponent* GetOwningVehicleComponent() const { return OwningVehicleComponent; }

	UFUNCTION()
	APilotCharacter* GetCurrentOccupant() const { return CurrentOccupant.Get(); }

	UFUNCTION()
	bool IsDriverSeat() const { return bDriverSeat; }

	virtual bool CanEnter(const APilotCharacter* PilotCharacter) const;
	virtual bool Enter(APilotCharacter* PilotCharacter);

	virtual bool Exit(APilotCharacter* PilotCharacter, EVehicleExitType Type);

	float GetSeatDistanceToSquared(const FVector& Location) const;
	const FName& GetSeatSocketName() const { return SeatSocketName; }
	const FVector& GetRelativeSeatLocation() const { return RelativeSeatLocation; }

protected:
	UFUNCTION()
	virtual void OnRep_CurrentOccupant();

protected:
	UPROPERTY(EditDefaultsOnly, Category = Seat)
	bool bDriverSeat = false;

	UPROPERTY(EditDefaultsOnly, Category = Seat)
	FName SeatSocketName = FName("Seat");

	UPROPERTY(Transient, ReplicatedUsing = OnRep_CurrentOccupant)
	TWeakObjectPtr<APilotCharacter> CurrentOccupant = nullptr;

	UPROPERTY(Transient)
	TWeakObjectPtr<APilotCharacter> PreviousOccupant = nullptr;

	UPROPERTY(Transient)
	UVehicleComponent* OwningVehicleComponent = nullptr;

	UPROPERTY(Transient)
	FVector RelativeSeatLocation = FAISystem::InvalidLocation;
};
