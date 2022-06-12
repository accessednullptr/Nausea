// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "Gameplay/StatusType.h"
#include "Gameplay/DamageLogInterface.h"
#include "CoreDamageType.generated.h"

class AActor;
class UStatusEffectBase;

UENUM(BlueprintType)
enum class EApplicationLogic : uint8
{
	Enemy, //Apply to enemies, but can friendly fire.
	EnemyOnly, //Apply to enemies only. Useful for enemy-only status effects.
	AllyOnly, //Apply to allies only. Useful for ally-only status effects.
	All //Apply to everyone.
};

UENUM(BlueprintType)
enum class EApplicationResult : uint8
{
	Full, //Event is fully applicable to target.
	FriendlyFire, //Event is friendly fire on target.
	None //Event should not apply to target.
};

/**
 * 
 */
UCLASS()
class UCoreDamageType : public UDamageType, public IDamageLogInterface
{
	GENERATED_UCLASS_BODY()

//~ Begin IDamageLogInterface Interface
public:
	virtual FText GetDamageLogInstigatorName() const { return DamageTypeName; }
//~ End IDamageLogInterface Interface

public:
	float GetDamageAmount() const { return DamageAmount; }

	float GetWeakpointDamageMultiplier() const { return WeakpointDamageMultiplier; }

	uint8 GetDamageHitDescriptors() const { return DamageHitDescriptors; }
	uint8 GetDamageElementalDescriptors() const { return DamageElementalDescriptors; }

	const TArray<EDamageHitDescriptor>& GetDamageHitDescriptorList() const;
	const TArray<EDamageElementalDescriptor>& GetDamageElementalDescriptorList() const;

	bool ShouldScaleStatusEffectPowerByDamage() const { return bScaleStatusEffectPowerByDamage; }

	bool ShouldScaleStatusEffectPowerByFrameDeltaTime() const { return bScaleStatusEffectPowerByFrameDeltaTime; }

	const TMap<TSoftClassPtr<UStatusEffectBase>, float>& GetStatusEffectMap() const { return StatusEffectMap; }

	const TMap<EStatusType, float>& GetGenericStatusEffectMap() const { return GenericStatusEffectMap; }

	EApplicationLogic GetDamageApplicationLogic() const { return DamageApplicationLogic; }
	EApplicationLogic GetStatusApplicationLogic() const { return StatusApplicationLogic; }

	EApplicationResult GetDamageApplicationResult(AActor* Instigator, AActor* Target) const;
	EApplicationResult GetStatusApplicationResult(AActor* Instigator, AActor* Target) const;

	bool ShouldIgnoreArmor() const { return bIgnoreArmor; }
	bool ShouldDecayArmor() const { return bDecayArmor; }
	float GetArmorDecayMultiplier() const { return ArmorDecayMultiplier; }

	bool ShouldBufferDamageLogEvent() const { return bBufferDamageLogEvent; }

	float GetThreatAmount(float Damage) const { return bOverrideThreatAmount ? ThreatAmountOverride : Damage * DamageThreatMultiplier; }

	float GetDeathNoiseMultiplier() const { return DeathNoiseMultiplier; }

	EHitReactionStrength GetHitReactionStrength(const FHitEvent& HitEvent) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = Logic)
	EApplicationLogic DamageApplicationLogic = EApplicationLogic::Enemy;
	UPROPERTY(EditDefaultsOnly, Category = Logic)
	EApplicationLogic StatusApplicationLogic = EApplicationLogic::EnemyOnly;

	UPROPERTY(EditDefaultsOnly, Category = Damage)
	float DamageAmount = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = Damage)
	float WeakpointDamageMultiplier = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = Damage, meta = (Bitmask, BitmaskEnum = EDamageHitDescriptor))
	uint8 DamageHitDescriptors = 0;
	UPROPERTY(EditDefaultsOnly, Category = Damage, meta = (Bitmask, BitmaskEnum = EDamageElementalDescriptor))
	uint8 DamageElementalDescriptors = 0;
	UPROPERTY(Transient)
	bool bInitializedTypeLists = false;
	UPROPERTY(Transient)
	TArray<EDamageHitDescriptor> DamageHitDescriptorList;
	UPROPERTY(Transient)
	TArray<EDamageElementalDescriptor> DamageElementalDescriptorList;

	UPROPERTY(EditDefaultsOnly, Category = Damage)
	bool bIgnoreArmor = false;

	UPROPERTY(EditDefaultsOnly, Category = Damage)
	bool bDecayArmor = true;

	UPROPERTY(EditDefaultsOnly, Category = Damage)
	float ArmorDecayMultiplier = 1.f;

	//Specific effect classes and the power that will be applied on hit of this damage type.
	UPROPERTY(EditDefaultsOnly, Category = StatusEffect)
	TMap<TSoftClassPtr<UStatusEffectBase>, float> StatusEffectMap;

	//Generic effect type and the power that will be applied on hit of this damage type. The specific effect class for this status type is dependent on the UStatusComponentConfigObject.
	UPROPERTY(EditDefaultsOnly, Category = StatusEffect)
	TMap<EStatusType, float> GenericStatusEffectMap;

	UPROPERTY(EditDefaultsOnly, Category = StatusEffect)
	bool bScaleStatusEffectPowerByDamage = false;

	//Should this effect power be multiplied by frame delta time?
	UPROPERTY(EditDefaultsOnly, Category = StatusEffect)
	bool bScaleStatusEffectPowerByFrameDeltaTime = false;

	UPROPERTY(EditDefaultsOnly, Category = DamageLog)
	FText DamageTypeName = FText();

	//If true will buffer this damage log event and group it with future matching damage logs. Enable for damage that is applied every frame.
	UPROPERTY(EditDefaultsOnly, Category = DamageLog)
	bool bBufferDamageLogEvent = false;

	//If bOverrideThreatAmount is false, threat applied will be incoming total damage multiplied by DamageThreatMultiplier.
	UPROPERTY(EditDefaultsOnly, Category = Perception, meta = (EditCondition = "!bOverrideThreatAmount"))
	float DamageThreatMultiplier = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = Perception)
	bool bOverrideThreatAmount = false;
	UPROPERTY(EditDefaultsOnly, Category = Perception, meta = (EditCondition = "bOverrideThreatAmount"))
	float ThreatAmountOverride = 0.f;

	//Modifies the amount of noise the target will make on death if this damage type is responsible for the kill.
	UPROPERTY(EditDefaultsOnly, Category = Perception)
	float DeathNoiseMultiplier = 1.f;

	//Decides what hit reaction strength a hit will play based on how much damage is applied.
	UPROPERTY(EditDefaultsOnly, Category = Damage)
	class UCurveFloat* HitReactionCurve = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = Experience)
	float DamageExperienceMultiplier = 1.f;
};
