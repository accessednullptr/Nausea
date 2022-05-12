// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gameplay/CoreDamageType.h"
#include "WeaponDamageType.generated.h"

class UWeapon;
class UFireMode;

/**
 * 
 */
UCLASS()
class NAUSEA_API UWeaponDamageType : public UCoreDamageType
{
	GENERATED_UCLASS_BODY()


public:
	UFUNCTION()
	virtual void RegisterOwningFireModeClass(const UFireMode* FireMode);

	UFUNCTION(BlueprintCallable, Category = DamageType)
	TSubclassOf<UWeapon> GetOwningWeaponClass() const { return OwningWeaponClass; }
	UFUNCTION(BlueprintCallable, Category = DamageType)
	TSubclassOf<UFireMode> GetOwningFireMode() const { return OwningFireModeClass; }

private:
	UPROPERTY(Transient)
	TSubclassOf<UWeapon> OwningWeaponClass;
	UPROPERTY(Transient)
	TSubclassOf<UFireMode> OwningFireModeClass;
};
