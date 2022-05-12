// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "Weapon/FireMode/ProjectileFireMode.h"
#include "GameFramework/Actor.h"
#include "Character/CoreCharacter.h"
#include "Weapon/Weapon.h"
#include "Weapon/FireMode/Projectile.h"

DECLARE_STATS_GROUP(TEXT("ProjectileFireMode"), STATGROUP_ProjectileFireMode, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Perform Projectile Spawn"), STAT_ProjectileFireModeSpawnProjectile, STATGROUP_ProjectileFireMode);

UProjectileFireMode::UProjectileFireMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UProjectileFireMode::PerformFire()
{
	if (IsLocallyOwned())
	{
		FVector FireLocation;
		FRotator FireRotation;
		GetOwningCharacter()->GetActorEyesViewPoint(FireLocation, FireRotation);

		if (!GetOwningWeapon()->IsAuthority())
		{
			Server_Reliable_FireProjectile(FireLocation, FireRotation.Vector());
			return;
		}
		else
		{
			LastReceivedLocation = FireLocation;
			LastReceivedDirection = FireRotation.Vector();
		}
	}

	SCOPE_CYCLE_COUNTER(STAT_ProjectileFireModeSpawnProjectile);

	if (bMakeNoiseOnFire)
	{
		FireNoise.MakeNoise(GetOwningCharacter());
	}

	FTransform ProjectileTransform = FTransform::Identity;
	ProjectileTransform.SetLocation(LastReceivedLocation);
	ProjectileTransform.SetRotation(LastReceivedDirection.ToOrientationQuat());
	AProjectile* Projectile = AProjectile::DeferredSpawnProjectile(this, this, ProjectileClass, ProjectileTransform);
	AProjectile::FinishDeferredSpawnProjectile(Projectile, ProjectileTransform);
}

bool UProjectileFireMode::Server_Reliable_FireProjectile_Validate(const FVector& Location, const FVector& Direction)
{
	return true;
}

void UProjectileFireMode::Server_Reliable_FireProjectile_Implementation(const FVector& Location, const FVector& Direction)
{
	LastReceivedLocation = Location;
	LastReceivedDirection = Direction;

	if (!IsLocallyOwned() && GetOwningController())
	{
		GetOwningController()->SetControlRotation(Direction.ToOrientationRotator());
	}
}