// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Weapon/FireMode/Projectile.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/GameStateBase.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/PhysicsVolume.h"
#include "Nausea.h"
#include "System/NetHelper.h"
#include "System/CoreGameplayStatics.h"
#include "Character/CoreCharacter.h"
#include "Player/CorePlayerState.h"
#include "Weapon/Weapon.h"
#include "Weapon/FireMode.h"
#include "Gameplay/CoreDamageType.h"

AProjectile::AProjectile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
}

void AProjectile::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	ResetReplicatedLifetimeProperty(StaticClass(), AActor::StaticClass(), GetInstigatorPropertyName(), COND_InitialOnly, OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(AProjectile, InstigatorPlayerState, PushReplicationParams::InitialOnly);
	DOREPLIFETIME_WITH_PARAMS_FAST(AProjectile, InstigatorTeamID, PushReplicationParams::InitialOnly);
}

void AProjectile::PostInitializeComponents()
{
	if (HasAuthority())
	{
		OnRep_Instigator();
	}

	Super::PostInitializeComponents();
}

void AProjectile::OnRep_Instigator()
{
	Super::OnRep_Instigator();

	APawn* InstigatorPawn = GetInstigator();

	if (!InstigatorPawn)
	{
		return;
	}

	InstigatorPawn->MoveIgnoreActorAdd(this);

	TInlineComponentArray<UPrimitiveComponent*> ProjectilePrimitiveList(this);
	for (UPrimitiveComponent* PrimitiveComponent : ProjectilePrimitiveList)
	{
		if (!PrimitiveComponent)
		{
			continue;
		}

		PrimitiveComponent->MoveIgnoreActors.Add(InstigatorPawn);
	}
}

void AProjectile::SetInstigatorFireMode(UFireMode* FireMode)
{
	if (!bDuringDeferredSpawn)
	{
		UE_LOG(LogFireMode, Error, TEXT("Projectile %s attempted to set instigating firemode %s after deferred spawning completed. To avoid any bugs, make sure to call SetInstigatorFireMode before calling FinishDeferredSpawnProjectile."),
			*GetNameSafe(this), *GetNameSafe(FireMode));
	}

	if (FireMode)
	{
		InstigatorFireMode = FireMode;
		InstigatorFireModeClass = FireMode->GetClass();

		if (FireMode->GetOwningWeapon())
		{
			InstigatorWeaponClass = FireMode->GetOwningWeapon()->GetClass();
		}
	}
}

AProjectile* AProjectile::DeferredSpawnProjectile(TScriptInterface<IPlayerOwnershipInterface> OwningInterface, UFireMode* OwningFireMode, TSubclassOf<AProjectile> ProjectileClass, const FTransform& Transform)
{
	if (!OwningInterface)
	{
		return nullptr;
	}

	UWorld* const World = GEngine->GetWorldFromContextObject(OwningInterface.GetObject(), EGetWorldErrorMode::LogAndReturnNull);

	if (!World)
	{
		return nullptr;
	}

	APawn* OwningInstigator = OwningInterface->GetOwningPawn();
	ACorePlayerState* OwningPlayerState = OwningInterface->GetOwningPlayerState();

	AProjectile* Projectile = World->SpawnActorDeferred<AProjectile>(ProjectileClass, Transform, OwningPlayerState, OwningInstigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!Projectile)
	{
		return nullptr;
	}

	Projectile->MarkDeferredSpawning();

	Projectile->InstigatorTeamID = OwningInterface->GetOwningTeamId();
	Projectile->InstigatorPlayerState = OwningPlayerState;
	MARK_PROPERTY_DIRTY_FROM_NAME(AProjectile, InstigatorTeamID, Projectile);
	MARK_PROPERTY_DIRTY_FROM_NAME(AProjectile, InstigatorPlayerState, Projectile);

	Projectile->SetInstigatorFireMode(OwningFireMode);

	Projectile->ProjectileDeferredSpawned();
	
	return Projectile;
}

void AProjectile::FinishDeferredSpawnProjectile(AProjectile* Projectile, const FTransform& Transform)
{
	if (!Projectile)
	{
		return;
	}

	Projectile->ClearDeferredSpawning();
	Projectile->FinishSpawning(Transform, true);
}

AProjectileSimple::AProjectileSimple(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void AProjectileSimple::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS(AProjectileSimple, AuthorityBounceHistory, PushReplicationParams::Default);
}

void AProjectileSimple::PostInitializeComponents()
{
	Velocity = GetActorForwardVector() * ProjectileSpeed;

	if (UPrimitiveComponent* PrimitiveRootComponent = Cast<UPrimitiveComponent>(GetRootComponent()))
	{
		PrimitiveRootComponent->OnComponentHit.AddDynamic(this, &AProjectileSimple::OnProjectileHit);
		PrimitiveRootComponent->OnComponentBeginOverlap.AddDynamic(this, &AProjectileSimple::OnProjectileOverlap);
	}

	Super::PostInitializeComponents();
}

void AProjectileSimple::TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction & ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	if (TickType == LEVELTICK_PauseTick || bHandledFinalImpact)
	{
		return;
	}

	PerformMovement(DeltaTime);
}

float AProjectileSimple::CalculateImpactDamage(const FHitResult& HitResult, uint8 PenentrationCount) const
{
	if (!ImpactDamageType)
	{
		return 0.f;
	}

	const UCoreDamageType* DamageTypeCDO = ImpactDamageType.GetDefaultObject();

	if (!DamageTypeCDO)
	{
		return 0.f;
	}

	float Damage = DamageTypeCDO->GetDamageAmount();

	if (PenetrationDamageFalloffCurve)
	{
		Damage *= PenetrationDamageFalloffCurve->GetFloatValue(float(PenetrationCount));
	}

	if (BounceDamageFalloffCurve)
	{
		Damage *= BounceDamageFalloffCurve->GetFloatValue(float(BounceCount));
	}

	return Damage;
}

void AProjectileSimple::PerformMovement(float DeltaTime)
{
	static uint8 IterationCount;
	IterationCount = 0;

	const float SubstepTime = DeltaTime / float(SubstepCount + 1);

	while (DeltaTime > 0.f && !FMath::IsNearlyZero(DeltaTime) && IterationCount++ < 50)
	{
		float StepDuration = FMath::Min(DeltaTime, SubstepTime);
		const FVector Delta = Velocity * StepDuration;

		if (!Delta.IsZero())
		{
			FHitResult SweepResult;
			GetRootComponent()->MoveComponent(Delta, GetActorRotation(), true, &SweepResult, MOVECOMP_NoFlags, ETeleportType::None);
			StepDuration *= SweepResult.Time;
		}

		Velocity.Z += GetRootComponent()->GetPhysicsVolume()->GetGravityZ() * GravityMultiplier * StepDuration;
		DeltaTime -= StepDuration;

		if (bHandledFinalImpact)
		{
			break;
		}
	}
}

bool AProjectileSimple::HandleProjectileOverlap(const FHitResult& HitResult)
{
	ApplyImpactDamage(HitResult);

	PenetrationCount++;

	if (PenetrationCount >= GetMaxPentrationCount())
	{
		HandleFinalProjectileImpact(HitResult);
		return true;
	}

	return false;
}

bool AProjectileSimple::HandleProjectileImpact(const FHitResult& HitResult)
{
	ApplyImpactDamage(HitResult);

	BounceCount++;

	if (BounceCount > GetMaxBounceCount())
	{
		HandleFinalProjectileImpact(HitResult);
		return true;
	}

	DrawDebugSphere(GetWorld(), GetActorLocation(), 10.f, 4, HasAuthority() ? FColor::Red : FColor::Green, true, 2.f, 0, 2.f);

	const FVector& ImpactNormal = HitResult.ImpactNormal;
	Velocity = Velocity.MirrorByVector(ImpactNormal) * BounceSpeedMultiplier;

	if (bReplicateBounceData)
	{
		const float ServerWorldTime = GetWorld()->GetGameState() ? GetWorld()->GetGameState()->GetServerWorldTimeSeconds() : 0.f;
		if (HasAuthority())
		{
			AuthorityBounceHistory.Add(FBounceHistory(ServerWorldTime, Velocity.Size(), Velocity.GetSafeNormal(), GetActorLocation()));
			MARK_PROPERTY_DIRTY_FROM_NAME(AProjectileSimple, AuthorityBounceHistory, this);
			ForceNetUpdate();
		}
		else
		{
			SimulatedBounceHistory.Add(FBounceHistory(ServerWorldTime, Velocity.Size(), Velocity.GetSafeNormal(), GetActorLocation()));
		}
	}

	return false;
}

void AProjectileSimple::HandleFinalProjectileImpact(const FHitResult& HitResult)
{
	Velocity = FVector::ZeroVector;
	bHandledFinalImpact = true;

	DrawDebugSphere(GetWorld(), GetActorLocation(), 20.f, 4, HasAuthority() ? FColor::Red : FColor::Green, true, 2.f, 0, 2.f);
	if (HasAuthority())
	{
		SetLifeSpan(0.01f);
	}
}

void AProjectileSimple::ApplyImpactDamage(const FHitResult& HitResult)
{
	AActor* HitActor = HitResult.GetActor();

	if (!HitActor)
	{
		return;
	}
	
	if (HitActor->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	const FVector ShotDirection = (HitResult.TraceEnd - HitResult.TraceStart).GetSafeNormal();

	UCoreGameplayStatics::ApplyWeaponPointDamage(HitResult.Actor.Get(), CalculateImpactDamage(HitResult, PenetrationCount),
		GetInstigatorWeaponClass(), GetClass(), ShotDirection, HitResult, GetOwningController(), this, ImpactDamageType);
}

void AProjectileSimple::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	HandleProjectileImpact(Hit);
}

void AProjectileSimple::OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	HandleProjectileImpact(SweepResult);
}

void AProjectileSimple::OnRep_AuthorityBounceHistory()
{
	if (HasAuthority())
	{
		return;
	}

	const float ServerWorldTime = GetWorld()->GetGameState() ? GetWorld()->GetGameState()->GetServerWorldTimeSeconds() : 0.f;

	for (int32 Index = 0; Index < AuthorityBounceHistory.Num(); Index++)
	{
		if (!SimulatedBounceHistory.IsValidIndex(Index))
		{
			FBounceHistory& AuthorityBounce = AuthorityBounceHistory[Index];

			//Prepare projectile for replay.
			SetActorLocation(AuthorityBounce.Location);
			Velocity = AuthorityBounce.Direction * AuthorityBounce.Speed;
			BounceCount = Index + 1;

			//Perform replay.
			const float TimeToReplay = FMath::Max(0.f, AuthorityBounce.WorldTime - ServerWorldTime);
			PerformMovement(TimeToReplay);
		}
	}
}

AProjectileSimpleExplosive::AProjectileSimpleExplosive(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void AProjectileSimpleExplosive::Explode()
{
	const FVector Location = GetActorLocation();

	if (ExplosiveInnerRadius <= 0.f && ExplosiveOuterRadius <= 0.f)
	{
		return;
	}

	//UCoreGameplayStatics::ApplyWeaponRadialDamageWithFalloff(this, )

	FCollisionShape Sphere = FCollisionShape::MakeSphere(ExplosiveInnerRadius + ExplosiveOuterRadius);

	//GetWorld()->OverlapAnyTestByChannel(Location, FQuat::Identity, ECC_Pawn, )
}