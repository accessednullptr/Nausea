// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenericTeamAgentInterface.h"
#include "Player/PlayerOwnershipInterface.h"
#include "Projectile.generated.h"

class ACorePlayerState;
class UWeapon;
class UFireMode;
class UCoreDamageType;

USTRUCT(BlueprintType)
struct FBounceHistory
{
	GENERATED_USTRUCT_BODY()
		
	FBounceHistory(float InWorldTime, float InSpeed, const FVector& InDirection, const FVector& InLocation)
	{
		WorldTime = InWorldTime;
		Speed = InSpeed;
		Direction = InDirection;
		Location = InLocation;
	}

	FBounceHistory() {}

	UPROPERTY()
	float WorldTime = -1.f;
	UPROPERTY()
	float Speed = 0.f;
	UPROPERTY()
	FVector_NetQuantizeNormal Direction = FVector_NetQuantizeNormal(ForceInitToZero);
	UPROPERTY()
	FVector_NetQuantize Location = FVector_NetQuantize(ForceInitToZero);
};

UCLASS(Blueprintable)
class NAUSEA_API AProjectile : public AActor, public IGenericTeamAgentInterface, public IPlayerOwnershipInterface
{
	GENERATED_UCLASS_BODY()

//~ Begin AActor Interface
protected:
	virtual void PostInitializeComponents() override;
public:
	virtual void OnRep_Instigator() override;
//~ End AActor Interface

//~ Begin IGenericTeamAgentInterface Interface
public:
	virtual FGenericTeamId GetGenericTeamId() const override { return InstigatorTeamID; }
//~ End IGenericTeamAgentInterface Interface

//~ Begin IPlayerOwnershipInterface Interface
public:
	virtual ACorePlayerState* GetOwningPlayerState() const override { return InstigatorPlayerState; }
	virtual APawn* GetOwningPawn() const override { return GetInstigator(); }
	virtual FGenericTeamId GetOwningTeamId() const override { return GetGenericTeamId(); }
//~ End IPlayerOwnershipInterface Interface

public:
	UFUNCTION()
	void SetInstigatorFireMode(UFireMode* FireMode);

	UFUNCTION()
	UFireMode* GetInstigatorFireMode() const { return InstigatorFireMode; }

	UFUNCTION()
	TSubclassOf<UWeapon> GetInstigatorWeaponClass() const { return InstigatorWeaponClass; }
	UFUNCTION()
	TSubclassOf<UFireMode> GetInstigatorFireModeClass() const { return InstigatorFireModeClass; }

protected:
	//Called at the end of AProjectile::DeferredSpawnProjectile.
	virtual void ProjectileDeferredSpawned() {}

protected:
	//No need for OnRep for these two properties, they are only sent in the initial bunch.
	UPROPERTY(Replicated)
	ACorePlayerState* InstigatorPlayerState = nullptr;
	UPROPERTY(Replicated)
	FGenericTeamId InstigatorTeamID = FGenericTeamId::NoTeam;

	//UFireModes get destroyed when weapons do, so we should avoid using this at all costs.
	UPROPERTY(Transient)
	UFireMode* InstigatorFireMode = nullptr;
	UPROPERTY(Transient)
	TSubclassOf<UWeapon> InstigatorWeaponClass = nullptr;
	UPROPERTY(Transient)
	TSubclassOf<UFireMode> InstigatorFireModeClass = nullptr;

	//Are we during deferred spawn time?
	UPROPERTY(Transient)
	bool bDuringDeferredSpawn = false;

private:
	void MarkDeferredSpawning() { bDuringDeferredSpawn = true; }
	void ClearDeferredSpawning() { bDuringDeferredSpawn = false; }

public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Projectile)
	static AProjectile* DeferredSpawnProjectile(TScriptInterface<IPlayerOwnershipInterface> OwningInterface, UFireMode* OwningFireMode, TSubclassOf<AProjectile> ProjectileClass, const FTransform& Transform);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Projectile)
	static void FinishDeferredSpawnProjectile(AProjectile* Projectile, const FTransform& Transform);
};

UCLASS()
class NAUSEA_API AProjectileSimple : public AProjectile
{
	GENERATED_UCLASS_BODY()

//~ Begin AActor Interface
protected:
	virtual void PostInitializeComponents() override;
public:
	virtual void TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
//~ End AActor Interface

public:
	UFUNCTION()
	uint8 GetMaxPentrationCount() const { return MaxPenetrationCount; }

	UFUNCTION()
	uint8 GetMaxBounceCount() const { return MaxBounceCount; }

	UFUNCTION()
	float CalculateImpactDamage(const FHitResult& HitResult, uint8 PenentrationCount) const;

protected:
	UFUNCTION()
	void PerformMovement(float DeltaTime);

	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	UFUNCTION()
	void OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	virtual bool HandleProjectileOverlap(const FHitResult& HitResult);
	UFUNCTION()
	virtual bool HandleProjectileImpact(const FHitResult& HitResult);
	UFUNCTION()
	virtual void HandleFinalProjectileImpact(const FHitResult& HitResult);

	virtual void ApplyImpactDamage(const FHitResult& HitResult);

	UFUNCTION()
	void OnRep_AuthorityBounceHistory();

protected:
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
	TSubclassOf<UCoreDamageType> ImpactDamageType = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
	uint8 MaxPenetrationCount = 0;
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
	class UCurveFloat* PenetrationDamageFalloffCurve = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
	class UCurveFloat* BounceDamageFalloffCurve = nullptr;

	UPROPERTY(Transient)
	uint8 PenetrationCount = 0;

	UPROPERTY(EditDefaultsOnly, Category = Movement)
	float ProjectileSpeed = 10000.f;
	//If true, we will keep speed at Projectile Speed after applying gravity.
	UPROPERTY(EditDefaultsOnly, Category = Movement)
	bool bMaintainSpeed = false;
	UPROPERTY(EditDefaultsOnly, Category = Movement)
	float GravityMultiplier = 1.f;
	UPROPERTY(EditDefaultsOnly, Category = Movement)
	uint8 SubstepCount = 1;

	UPROPERTY(EditDefaultsOnly, Category = Movement)
	class UCurveFloat* DistanceDamageFalloffCurve = nullptr;

	UPROPERTY(Transient)
	uint8 BounceCount = 0;

	UPROPERTY(EditDefaultsOnly, Category = Movement)
	uint8 MaxBounceCount = 0;
	UPROPERTY(EditDefaultsOnly, Category = Movement)
	float BounceSpeedMultiplier = 1.f;
	UPROPERTY(EditDefaultsOnly, Category = Movement)
	bool bReplicateBounceData = false;

	UPROPERTY(Transient)
	FVector Velocity = FVector(0);
	
	UPROPERTY(Transient)
	bool bHandledFinalImpact = false;

	UPROPERTY(Transient)
	TArray<FBounceHistory> SimulatedBounceHistory;
	UPROPERTY(ReplicatedUsing = OnRep_AuthorityBounceHistory)
	TArray<FBounceHistory> AuthorityBounceHistory;
};

UCLASS()
class NAUSEA_API AProjectileSimpleExplosive : public AProjectileSimple
{
	GENERATED_UCLASS_BODY()

public:
	virtual float GetExplosiveRadius() const { return FMath::Max(ExplosiveInnerRadius, ExplosiveOuterRadius); }
	virtual float GetExplosiveInnerRadius() const { return ExplosiveInnerRadius; }
	virtual float GetExplosiveOuterRadius() const { return ExplosiveOuterRadius; }

	bool HasExploded() const { return bHasExploded; }

protected:
	virtual void Explode();

protected:
	UPROPERTY(EditDefaultsOnly, Category = Explosive)
	float ExplosiveInnerRadius = 0.f;
	UPROPERTY(EditDefaultsOnly, Category = Explosive)
	float ExplosiveOuterRadius = 0.f;

	UPROPERTY(Transient)
	bool bHasExploded = false;
};