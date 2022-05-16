
#include "AI/CoreAITypes.h"

namespace CoreNoiseTag
{
	const FName Generic = FName(TEXT("GenericNoise"));

	const FName InventoryPickup = FName(TEXT("PickupNoise"));
	const FName InventoryDrop = FName(TEXT("DropNoise"));

	const FName WeaponEquip = FName(TEXT("EquipNoise"));
	const FName WeaponPutDown = FName(TEXT("PutDownNoise"));
	const FName WeaponFire = FName(TEXT("FireNoise"));
	const FName WeaponReload = FName(TEXT("ReloadNoise"));
	const FName MeleeSwing = FName(TEXT("MeleeSwingNoise"));
	const FName MeleeHit = FName(TEXT("MeleeHitNoise"));

	const FName Damage = FName(TEXT("DamageNoise"));
	const FName Death = FName(TEXT("DeathNoise"));
}

bool FCoreNoiseParams::MakeNoise(AActor* Instigator, const FVector& Location, float LoudnessMultiplier) const
{
	if (!Instigator || Instigator->IsPendingKillPending())
	{
		return false;
	}

	Instigator->MakeNoise(Loudness * LoudnessMultiplier, Cast<APawn>(Instigator), Location, MaxRadius, (Tag == NAME_None ? CoreNoiseTag::Generic : Tag));
	return true;
}