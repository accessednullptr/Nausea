// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Weapon/InventoryPickup.h"

AInventoryPickup::AInventoryPickup(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AInventoryPickup::BeginPlay()
{
	Super::BeginPlay();
	
}

void AInventoryPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}