// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Gameplay/StatusEffect/StatusEffectBase.h"
#include "Gameplay/DamageLogModifier/DamageLogModifierObject.h"
#include "StatusEffectShield.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API UStatusEffectShield : public UStatusEffectBasic
{
	GENERATED_UCLASS_BODY()

//~ Begin UStatusEffectBase Interface
public:
	virtual void OnActivated(EStatusBeginType BeginType) override;
//~ End UStatusEffectBase Interface

//~ Begin UStatusEffectBasic Interface
public:
	virtual float GetDurationAtCurrentPower() const override;
protected:
	virtual void ProcessDamage(UStatusComponent* Component, float& Damage, const struct FDamageEvent& DamageEvent, ACorePlayerState* Instigator) override;
//~ End UStatusEffectBasic Interface

//~ Begin FTickableGameObject Interface
protected:
	virtual void Tick(float DeltaTime) override;
//~ End FTickableGameObject Interface

protected:
	UPROPERTY(Transient)
	float CurrentShieldAmount = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = Shield)
	float ShieldAmount = 100.f;

	//Used to describe armor damage modifications to the damage log.
	UPROPERTY(EditDefaultsOnly, Category = Shield)
	TSubclassOf<UDamageLogModifierObject> ShieldDamageLogModifierClass;
};

UCLASS()
class NAUSEA_API UStatusEffectShieldDamageLogModifier : public UDamageLogModifierObject
{
	GENERATED_UCLASS_BODY()
};