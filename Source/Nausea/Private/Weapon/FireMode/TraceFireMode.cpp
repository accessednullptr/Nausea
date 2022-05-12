// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "Weapon/FireMode/TraceFireMode.h"
#include "Character/CoreCharacter.h"
#include "Weapon/Weapon.h"
#include "Gameplay/CoreDamageType.h"
#include "System/CoreGameplayStatics.h"

DECLARE_STATS_GROUP(TEXT("TraceFireMode"), STATGROUP_TraceFireMode, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Perform Trace"), STAT_TraceFireModePerformTrace, STATGROUP_TraceFireMode);
DECLARE_CYCLE_STAT(TEXT("Process Hits"), STAT_TraceFireModeProcessHits, STATGROUP_TraceFireMode);

FTraceHitResult::operator FHitResult() const
{
	FHitResult HitResult = FHitResult();
	HitResult.bBlockingHit = false;
	HitResult.bStartPenetrating = false;
	HitResult.FaceIndex = 0;

	HitResult.Time = Distance / (TraceStart - TraceEnd).Size();
	HitResult.Distance = Distance;

	HitResult.Location = ImpactLocation;
	HitResult.ImpactPoint = ImpactLocation;

	HitResult.Normal = ImpactNormal;
	HitResult.ImpactNormal = ImpactNormal;

	HitResult.TraceStart = TraceStart;
	HitResult.TraceEnd = TraceEnd;

	HitResult.PenetrationDepth = 0.f;

	HitResult.Item = Item;
	HitResult.ElementIndex = ElementIndex;

	HitResult.PhysMaterial = PhysMaterial;
	HitResult.Actor = Actor;
	HitResult.Component = Component;

	HitResult.BoneName = BoneName;
	HitResult.MyBoneName = NAME_None;

	return MoveTemp(HitResult);
}

FTraceHitResult::FTraceHitResult(const FHitResult& HitResult)
{
	Distance = HitResult.Distance;
	ImpactLocation = HitResult.ImpactPoint;
	ImpactNormal = HitResult.ImpactNormal;

	TraceStart = HitResult.TraceStart;
	TraceEnd = HitResult.TraceEnd;

	Actor = HitResult.Actor;
	Component = HitResult.Component;

	Item = HitResult.Item;
	ElementIndex = HitResult.ElementIndex;

	BoneName = HitResult.BoneName;

	PhysMaterial = HitResult.PhysMaterial;
}

UTraceFireMode::UTraceFireMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

static FVector InvalidVector = FVector(MAX_FLT);
bool UTraceFireMode::Fire(float WorldTimeOverride)
{
	if (!Super::Fire(WorldTimeOverride))
	{
		LastReceivedHitList.Empty();
		LastReceivedLocation = InvalidVector;
		LastReceivedDirection = InvalidVector;
		return false;
	}

	return true;
}

static TArray<FHitResult> HitResultList;
void UTraceFireMode::PerformFire()
{
	Super::PerformFire();

	if (GetOwningWeapon()->IsAuthority() && bMakeNoiseOnFire)
	{
		FireNoise.MakeNoise(GetOwningCharacter());
	}
	
	if (IsLocallyOwned())
	{
		SCOPE_CYCLE_COUNTER(STAT_TraceFireModePerformTrace);

		//Push location and rotation data right before sending fire. Ensures the authority will be current if it needs to perform hit detection / needs to play effects in some capacity.
		const FVector FireLocation = GetFireLocation();
		const FVector FireDirection = GetFireDirection();

		if (FireLocation == InvalidVector
			|| FireDirection == InvalidVector)
		{
			return;
		}

		FCollisionQueryParams CollisionParams = FCollisionQueryParams::DefaultQueryParam;
		CollisionParams.AddIgnoredActor(GetOwningCharacter());

		if (GetMaxPenetrationCount() == 0)
		{
			HitResultList.SetNumZeroed(1, false);
			GetWorld()->LineTraceSingleByChannel(HitResultList[0], FireLocation, FireLocation + (FireDirection * TraceLength), ECC_WeaponTrace, CollisionParams);
		}
		else
		{
			GetWorld()->LineTraceMultiByChannel(HitResultList, FireLocation, FireLocation + (FireDirection * TraceLength), ECC_WeaponTrace, CollisionParams);
		}

		static TArray<FTraceHitResult> TraceHitResultList;
		TraceHitResultList.Empty(FMath::Max(HitResultList.Num(), TraceHitResultList.Num()));

		for (const FHitResult& HitResult : HitResultList)
		{
			TraceHitResultList.Add(HitResult);
		}

		HitResultList.Empty(HitResultList.Num());

		if (TraceHitResultList.Num() == 0)
		{
			return;
		}

		TraceHitResultList.SetNum(FMath::Min(TraceHitResultList.Num(), GetMaxPenetrationCount() + 1), false);
		TraceHitResultList.Sort(FTraceHitResult::FSortByDistance());

		if (!GetOwningWeapon()->IsAuthority())
		{
			int32 PenetrationCount = 0;
			for (const FTraceHitResult& TraceHitResult : TraceHitResultList)
			{
				if (!TraceHitResult.Actor.IsValid())
				{
					continue;
				}

				//We can damage things that we are the authority of or that have been torn off.
				if (TraceHitResult.Actor->GetLocalRole() != ROLE_Authority && !TraceHitResult.Actor->GetTearOff())
				{
					continue;
				}

				const FVector ShotDirection = (TraceHitResult.TraceEnd - TraceHitResult.TraceStart).GetSafeNormal();

				UCoreGameplayStatics::ApplyWeaponPointDamage(TraceHitResult.Actor.Get(), CalculateDamage(TraceHitResult, PenetrationCount),
					GetOwningWeapon()->GetClass(), GetClass(), ShotDirection, TraceHitResult, GetOwningController(), GetOwningCharacter(), DamageTypeClass);

				PenetrationCount++;
			}

			Server_Reliable_FireTrace(FireLocation, FireDirection, TraceHitResultList);
			return;
		}
		else //If on the authority, immediately process this data into the last received cache and continue through PerformFire.
		{
			LastReceivedLocation = FireLocation;
			LastReceivedDirection = FireDirection;
			LastReceivedHitList = TraceHitResultList;
		}
	}

	//TODO: Maybe redo the trace if the client's ping is sufficiently high?

	if (LastReceivedLocation == InvalidVector
		|| LastReceivedDirection == InvalidVector)
	{
		return;
	}

	if (LastReceivedHitList.Num() == 0)
	{
		return;
	}

	SCOPE_CYCLE_COUNTER(STAT_TraceFireModeProcessHits);

	const FVector HitTraceStart = LastReceivedHitList[0].TraceStart;
	const FVector HitTraceEnd = LastReceivedHitList[0].TraceEnd;

	for (FTraceHitResult& TraceHitResult : LastReceivedHitList)
	{
		const float RecalculatedHitDistance = (LastReceivedLocation - TraceHitResult.ImpactLocation).Size();

		//Distance mismatch found in hit detection received.
		if (!FMath::IsNearlyEqual(RecalculatedHitDistance, TraceHitResult.Distance, 1.f))
		{
			LastReceivedHitList.Empty();
			LastReceivedLocation = InvalidVector;
			LastReceivedDirection = InvalidVector;
			return;
		}

		//Trace data mismatch found.
		if (HitTraceStart != TraceHitResult.TraceStart
			|| HitTraceEnd != TraceHitResult.TraceEnd)
		{
			LastReceivedHitList.Empty();
			LastReceivedLocation = InvalidVector;
			LastReceivedDirection = InvalidVector;
			return;
		}
		
		TraceHitResult.Distance = RecalculatedHitDistance;
	}

	LastReceivedHitList.Sort(FTraceHitResult::FSortByDistance());
	
	int32 PenetrationCount = 0;
	for (const FTraceHitResult& TraceHitResult : LastReceivedHitList)
	{
		if (!TraceHitResult.Actor.IsValid())
		{
			continue;
		}
		
		const FVector ShotDirection = (TraceHitResult.TraceEnd - TraceHitResult.TraceStart).GetSafeNormal();

		UCoreGameplayStatics::ApplyWeaponPointDamage(TraceHitResult.Actor.Get(), CalculateDamage(TraceHitResult, PenetrationCount),
			GetOwningWeapon()->GetClass(), GetClass(), ShotDirection, TraceHitResult, GetOwningController(), GetOwningCharacter(), DamageTypeClass);

		PenetrationCount++;
	}

	LastReceivedHitList.Empty();
	LastReceivedLocation = InvalidVector;
	LastReceivedDirection = InvalidVector;
}

float UTraceFireMode::CalculateDamage(const FTraceHitResult& TraceHit, int32 PenetrationCount) const
{
	if (!DamageTypeClass)
	{
		return 0.f;
	}

	const UCoreDamageType* DamageTypeCDO = DamageTypeClass.GetDefaultObject();

	if (!DamageTypeCDO)
	{
		return 0.f;
	}

	float Damage = DamageTypeClass.GetDefaultObject()->GetDamageAmount();

	Damage *= DistanceDamageFalloffCurve ? DistanceDamageFalloffCurve->GetFloatValue(TraceHit.Distance) : 1.f;
	Damage *= PenetrationDamageFalloffCurve ? PenetrationDamageFalloffCurve->GetFloatValue(PenetrationCount) : 1.f;

	return Damage;
}

bool UTraceFireMode::Server_Reliable_FireTrace_Validate(const FVector& Location, const FVector& Direction, const TArray<FTraceHitResult>& TraceHitResultList)
{
	return TraceHitResultList.Num() <= (GetMaxPenetrationCount() + 1); //Forged requests with a larger number of trace hits than possible should result in a kick.
}

void UTraceFireMode::Server_Reliable_FireTrace_Implementation(const FVector& Location, const FVector& Direction, const TArray<FTraceHitResult>& TraceHitResultList)
{
	ReceiveTraceData(Location, Direction, TraceHitResultList);
}

void UTraceFireMode::ReceiveTraceData(const FVector& Location, const FVector& Direction, const TArray<FTraceHitResult>& TraceHitResultList)
{
	LastReceivedLocation = Location;
	LastReceivedDirection = Direction;
	LastReceivedHitList = TraceHitResultList;
	
	//Update control rotation immediately if we're the remote authority. Purely to keep view rotation visually up to date.
	if (!IsLocallyOwned() && GetOwningController())
	{
		GetOwningController()->SetControlRotation(Direction.ToOrientationRotator());
	}
}