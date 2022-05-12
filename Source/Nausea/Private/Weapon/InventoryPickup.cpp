// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


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