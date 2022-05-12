// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#include "Gameplay/WeaponDamageType.h"
#include "Weapon/FireMode.h"

UWeaponDamageType::UWeaponDamageType(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UWeaponDamageType::RegisterOwningFireModeClass(const UFireMode* FireMode)
{
	if (!FireMode)
	{
		return;
	}

	OwningFireModeClass = FireMode->GetClass();
	OwningWeaponClass = FireMode->GetOwningWeaponClass();
}