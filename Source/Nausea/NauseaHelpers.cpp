// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#include "NauseaHelpers.h"
#include "System/NetHelper.h"

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

namespace PushReplicationParams
{
	const FDoRepLifetimeParams Default{ COND_None, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams InitialOnly{ COND_InitialOnly, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams OwnerOnly{ COND_OwnerOnly, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams SkipOwner{ COND_SkipOwner, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams SimulatedOnly{ COND_SimulatedOnly, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams AutonomousOnly{ COND_AutonomousOnly, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams SimulatedOrPhysics{ COND_SimulatedOrPhysics, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams InitialOrOwner{ COND_InitialOrOwner, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams Custom{ COND_Custom, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams ReplayOrOwner{ COND_ReplayOrOwner, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams ReplayOnly{ COND_ReplayOnly, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams SimulatedOnlyNoReplay{ COND_SimulatedOnlyNoReplay, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams SimulatedOrPhysicsNoReplay{ COND_SimulatedOrPhysicsNoReplay, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams SkipReplay{ COND_SkipReplay, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams Never{ COND_Never, REPNOTIFY_OnChanged, true };
}