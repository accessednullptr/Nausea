// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Weapon/WeaponAnimInstance.h"
#include "Character/CoreCharacter.h"
#include "Weapon/InventoryManagerComponent.h"
#include "Weapon/Weapon.h"
#include "Weapon/FireMode/MeleeFireMode.h"

uint64 FMeleeNotifyDataHandle::HandleIDCounter = MAX_uint64;

UWeaponAnimInstance::UWeaponAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UWeaponAnimInstance::RegisterOwningWeapon(UWeapon* Weapon)
{
	OwningWeapon = Weapon;
}

void UWeaponAnimInstance::ProcessMeleeNotifyHitBox(const FMeleeNotifyData& Data)
{
	OnProcessMeleeHitboxEvent.Broadcast(this, Data);
}

void UWeaponAnimInstance::OnFireStart(UMeleeFireMode* FireMode)
{
	if (FireModeBroadcastHandle.IsValid())
	{
		OnProcessMeleeHitboxEvent.Remove(FireModeBroadcastHandle);
	}

	TWeakObjectPtr<UMeleeFireMode> WeakFireMode = FireMode;
	OnProcessMeleeHitboxEvent.AddWeakLambda(this, [WeakFireMode](UWeaponAnimInstance* AnimInstance, const FMeleeNotifyData& Data)
	{
		if (!WeakFireMode.IsValid() || !AnimInstance || AnimInstance->IsPendingKill())
		{
			return;
		}

		WeakFireMode->ProcessHitbox(AnimInstance, Data);
	});
}

void UWeaponAnimInstance::OnFireStop(UMeleeFireMode* FireMode)
{
	if (FireModeBroadcastHandle.IsValid())
	{
		OnProcessMeleeHitboxEvent.Remove(FireModeBroadcastHandle);
	}

	//For now we'll clear all bound events just to be sure nothing weird happens between fires.
	OnProcessMeleeHitboxEvent.Clear();
}