// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Weapon/FireMode/MeleeFireMode.h"
#include "Weapon/Weapon.h"
#include "Weapon/WeaponAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

UMeleeFireMode::UMeleeFireMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

bool UMeleeFireMode::Fire(float WorldTimeOverride)
{
	if (!Super::Fire(WorldTimeOverride))
	{
		return false;
	}



	return true;
}

void UMeleeFireMode::PerformFire()
{
	Super::PerformFire();

	if (GetOwningWeapon()->IsAuthority() && bMakeNoiseOnSwing)
	{
		SwingNoise.MakeNoise(GetOwningPawn());
	}

	if (IsLocallyOwned())
	{
		if (USkeletalMeshComponent* Mesh1P = GetOwningWeapon()->GetMesh1P())
		{
			if (UWeaponAnimInstance* WeaponAnimInstance = Cast<UWeaponAnimInstance>(Mesh1P->GetAnimInstance()))
			{
				WeaponAnimInstance->OnFireStart(this);
			}
		}
	}
}

void UMeleeFireMode::FireComplete()
{
	if (IsLocallyOwned())
	{
		if (USkeletalMeshComponent* Mesh1P = GetOwningWeapon()->GetMesh1P())
		{
			if (UWeaponAnimInstance* WeaponAnimInstance = Cast<UWeaponAnimInstance>(Mesh1P->GetAnimInstance()))
			{
				WeaponAnimInstance->OnFireStop(this);
			}
		}
	}

	Super::FireComplete();
}

void UMeleeFireMode::ProcessHitbox(UWeaponAnimInstance* AnimationInstance, const FMeleeNotifyData& Hitbox)
{

}