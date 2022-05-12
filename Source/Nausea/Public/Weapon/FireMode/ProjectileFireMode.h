// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/FireMode/WeaponFireMode.h"
#include "ProjectileFireMode.generated.h"

class AProjectile;

/**
 * 
 */
UCLASS()
class NAUSEA_API UProjectileFireMode : public UWeaponFireMode
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UFireMode Interface 
protected:
	virtual void PerformFire() override;
//~ End UFireMode Interface 

protected:
	//Payload containing trace data.
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Reliable_FireProjectile(const FVector& Location, const FVector& Direction);

protected:
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
	TSubclassOf<AProjectile> ProjectileClass = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bMakeNoiseOnFire = false;
	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (EditCondition = "bMakeNoiseOnFire", DisplayName = "Make Noise On Fire"))
	FCoreNoiseParams FireNoise = FCoreNoiseParams(CoreNoiseTag::WeaponFire, 2.f, 0.f);

	UPROPERTY(Transient)
	FVector LastReceivedLocation = FVector(MAX_FLT);
	UPROPERTY(Transient)
	FVector LastReceivedDirection = FVector(MAX_FLT);
};
