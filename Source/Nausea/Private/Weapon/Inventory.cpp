// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Weapon/Inventory.h"
#include "Character/CoreCharacter.h"
#include "Weapon/InventoryManagerComponent.h"

UInventory::UInventory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void UInventory::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		return;
	}

	OwningInventoryManager = GetOwningCharacter()->GetInventoryManager();
}

void UInventory::SetMovementSpeedModifier(float InMovementSpeedModifier)
{
	MovementSpeedModifier = InMovementSpeedModifier;
	if (GetOwningInventoryManager())
	{
		GetOwningInventoryManager()->RequestMovementSpeedModifierUpdate();
	}
}

void UInventory::InventoryPickedUp()
{
	if (bMakeNoiseOnPickup)
	{
		PickupNoise.MakeNoise(GetOwningCharacter());
	}
}

void UInventory::InventoryDropped()
{
	if (bMakeNoiseOnDrop)
	{
		DropNoise.MakeNoise(GetOwningCharacter());
	}
}