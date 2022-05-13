// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "System/CoreGameplayStatics.h"
#include "GameFramework/Actor.h"
#include "Gameplay/CoreDamageType.h"
#include "Gameplay/StatusType.h"
#include "Weapon/Weapon.h"
#include "Weapon/FireMode.h"
#include "GeneralProjectSettings.h"

UCoreGameplayStatics::UCoreGameplayStatics(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FString UCoreGameplayStatics::GetNetRoleNameForActor(AActor* Actor)
{
	if (Actor)
	{
		return GetNetRoleName(Actor->GetLocalRole());
	}

	return GetNetRoleName(ROLE_None);
}

FString UCoreGameplayStatics::GetNetRoleName(ENetRole Role)
{
	switch (Role)
	{
	case ROLE_Authority:
		return "AUTHORITY";
	case ROLE_AutonomousProxy:
		return "AUTONOMOUS";
	case ROLE_SimulatedProxy:
		return "SIMULATED";
	case ROLE_None:
		return "NONE";
	}

	return "NONE";
}

FString UCoreGameplayStatics::GetProjectVersion()
{
	return GetDefault<UGeneralProjectSettings>()->ProjectVersion;
}

/** @RETURN True if weapon trace from Origin hits component VictimComp.  OutHitResult will contain properties of the hit. */
static bool ComponentIsDamageableFrom(UPrimitiveComponent* VictimComp, FVector const& Origin, AActor const* IgnoredActor, const TArray<AActor*>& IgnoreActors, ECollisionChannel TraceChannel, FHitResult& OutHitResult)
{
	FCollisionQueryParams LineParams(SCENE_QUERY_STAT(ComponentIsVisibleFrom), true, IgnoredActor);
	LineParams.AddIgnoredActors(IgnoreActors);

	// Do a trace from origin to middle of box
	UWorld* const World = VictimComp->GetWorld();
	check(World);

	FVector const TraceEnd = VictimComp->Bounds.Origin;
	FVector TraceStart = Origin;
	if (Origin == TraceEnd)
	{
		// tiny nudge so LineTraceSingle doesn't early out with no hits
		TraceStart.Z += 0.01f;
	}

	// Only do a line trace if there is a valid channel, if it is invalid then result will have no fall off
	if (TraceChannel != ECollisionChannel::ECC_MAX)
	{
		bool const bHadBlockingHit = World->LineTraceSingleByChannel(OutHitResult, TraceStart, TraceEnd, TraceChannel, LineParams);
		//::DrawDebugLine(World, TraceStart, TraceEnd, FLinearColor::Red, true);

		// If there was a blocking hit, it will be the last one
		if (bHadBlockingHit)
		{
			if (OutHitResult.Component == VictimComp)
			{
				// if blocking hit was the victim component, it is visible
				return true;
			}
			else
			{
				// if we hit something else blocking, it's not
				UE_LOG(LogDamage, Log, TEXT("Radial Damage to %s blocked by %s (%s)"), *GetNameSafe(VictimComp), *GetNameSafe(OutHitResult.GetActor()), *GetNameSafe(OutHitResult.Component.Get()));
				return false;
			}
		}
	}
	else
	{
		UE_LOG(LogDamage, Warning, TEXT("ECollisionChannel::ECC_MAX is not valid! No falloff is added to damage"));
	}

	// didn't hit anything, assume nothing blocking the damage and victim is consequently visible
	// but since we don't have a hit result to pass back, construct a simple one, modeling the damage as having hit a point at the component's center.
	FVector const FakeHitLoc = VictimComp->GetComponentLocation();
	FVector const FakeHitNorm = (Origin - FakeHitLoc).GetSafeNormal();		// normal points back toward the epicenter
	OutHitResult = FHitResult(VictimComp->GetOwner(), VictimComp, FakeHitLoc, FakeHitNorm);
	return true;
}

bool UCoreGameplayStatics::ApplyWeaponRadialDamage(const UObject* WorldContextObject, float BaseDamage, TSubclassOf<UWeapon> WeaponClass, TSubclassOf<UFireMode> FireModeClass, const FVector& Origin, float DamageRadius, TSubclassOf<UDamageType> DamageTypeClass, const TArray<AActor*>& IgnoreActors, AActor* DamageCauser, AController* InstigatedByController, bool bDoFullDamage, ECollisionChannel DamagePreventionChannel)
{
	float DamageFalloff = bDoFullDamage ? 0.f : 1.f;
	return ApplyWeaponRadialDamageWithFalloff(WorldContextObject, BaseDamage, WeaponClass, FireModeClass, 0.f, Origin, 0.f, DamageRadius, DamageFalloff, DamageTypeClass, IgnoreActors, DamageCauser, InstigatedByController, DamagePreventionChannel);
}
	
bool UCoreGameplayStatics::ApplyWeaponRadialDamageWithFalloff(const UObject* WorldContextObject, float BaseDamage, TSubclassOf<UWeapon> WeaponClass, TSubclassOf<UFireMode> FireModeClass, float MinimumDamage, const FVector& Origin, float DamageInnerRadius, float DamageOuterRadius, float DamageFalloff, TSubclassOf<UDamageType> DamageTypeClass, const TArray<AActor*>& IgnoreActors, AActor* DamageCauser, AController* InstigatedByController, ECollisionChannel DamagePreventionChannel)
{
	FCollisionQueryParams SphereParams(SCENE_QUERY_STAT(ApplyRadialDamage), false, DamageCauser);

	SphereParams.AddIgnoredActors(IgnoreActors);

	// query scene to see what we hit
	TArray<FOverlapResult> Overlaps;
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllDynamicObjects), FCollisionShape::MakeSphere(DamageOuterRadius), SphereParams);
	}

	// collate into per-actor list of hit components
	TMap<AActor*, TArray<FHitResult> > OverlapComponentMap;
	for (int32 Idx = 0; Idx < Overlaps.Num(); ++Idx)
	{
		FOverlapResult const& Overlap = Overlaps[Idx];
		AActor* const OverlapActor = Overlap.GetActor();

		if (OverlapActor &&
			OverlapActor->CanBeDamaged() &&
			OverlapActor != DamageCauser &&
			Overlap.Component.IsValid())
		{
			FHitResult Hit;
			if (ComponentIsDamageableFrom(Overlap.Component.Get(), Origin, DamageCauser, IgnoreActors, DamagePreventionChannel, Hit))
			{
				TArray<FHitResult>& HitList = OverlapComponentMap.FindOrAdd(OverlapActor);
				HitList.Add(Hit);
			}
		}
	}

	bool bAppliedDamage = false;

	if (OverlapComponentMap.Num() > 0)
	{
		// make sure we have a good damage type
		TSubclassOf<UDamageType> const ValidDamageTypeClass = DamageTypeClass ? DamageTypeClass : TSubclassOf<UCoreDamageType>(UCoreDamageType::StaticClass());

		FCoreRadialDamageEvent RadialDamageEvent;
		RadialDamageEvent.DamageTypeClass = ValidDamageTypeClass;
		RadialDamageEvent.Origin = Origin;
		RadialDamageEvent.Params = FRadialDamageParams(BaseDamage, MinimumDamage, DamageInnerRadius, DamageOuterRadius, DamageFalloff);
		RadialDamageEvent.WeaponClass = WeaponClass;
		RadialDamageEvent.FireModeClass = FireModeClass;

		// call damage function on each affected actors
		for (TMap<AActor*, TArray<FHitResult> >::TIterator It(OverlapComponentMap); It; ++It)
		{
			AActor* const Victim = It.Key();
			TArray<FHitResult> const& ComponentHits = It.Value();
			RadialDamageEvent.ComponentHits = ComponentHits;

			Victim->TakeDamage(BaseDamage, RadialDamageEvent, InstigatedByController, DamageCauser);

			bAppliedDamage = true;
		}
	}

	return bAppliedDamage;
}

float UCoreGameplayStatics::ApplyWeaponPointDamage(AActor* DamagedActor, float BaseDamage, TSubclassOf<UWeapon> WeaponClass, TSubclassOf<UFireMode> FireModeClass, const FVector& HitFromDirection, const FHitResult& HitInfo, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass)
{
	if (DamagedActor)
	{
		TSubclassOf<UDamageType> const ValidDamageTypeClass = DamageTypeClass ? DamageTypeClass : TSubclassOf<UCoreDamageType>(UCoreDamageType::StaticClass());
		FCorePointDamageEvent PointDamageEvent(BaseDamage, HitInfo, HitFromDirection, ValidDamageTypeClass);
		PointDamageEvent.WeaponClass = WeaponClass;
		PointDamageEvent.FireModeClass = FireModeClass;

		return DamagedActor->TakeDamage(BaseDamage, PointDamageEvent, EventInstigator, DamageCauser);
	}

	return 0.f;
}

float UCoreGameplayStatics::ApplyWeaponDamage(AActor* DamagedActor, float BaseDamage, TSubclassOf<UWeapon> WeaponClass, TSubclassOf<UFireMode> FireModeClass, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass)
{
	if (DamagedActor)
	{
		TSubclassOf<UDamageType> const ValidDamageTypeClass = DamageTypeClass ? DamageTypeClass : TSubclassOf<UCoreDamageType>(UCoreDamageType::StaticClass());
		FCoreDamageEvent DamageEvent(ValidDamageTypeClass);
		DamageEvent.WeaponClass = WeaponClass;
		DamageEvent.FireModeClass = FireModeClass;

		return DamagedActor->TakeDamage(BaseDamage, DamageEvent, EventInstigator, DamageCauser);
	}

	return 0.f;
}