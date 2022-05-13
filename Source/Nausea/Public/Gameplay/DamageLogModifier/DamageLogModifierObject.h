// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Gameplay/DamageLogInterface.h"
#include "DamageLogModifierObject.generated.h"

/**
 * Used to define arbitrary damage log modifiers that can't/shouldn't be using the actual class/instance that applies the modification as the instigator (such as armor)
 */
UCLASS(HideFunctions = (K2_GetDamageLogInstigatorName))
class NAUSEA_API UDamageLogModifierObject : public UObject, public IDamageLogInterface
{
	GENERATED_UCLASS_BODY()

//~ Begin IDamageLogInterface Interface
public:
	virtual FText GetDamageLogInstigatorName() const override { return InstigatorName; }
//~ End IDamageLogInterface Interface

protected:
	UPROPERTY(EditDefaultsOnly, Category = DamageLogModifier)
	FText InstigatorName = FText::GetEmpty();
};

UCLASS()
class NAUSEA_API UArmorDamageLogModifier : public UDamageLogModifierObject
{
	GENERATED_UCLASS_BODY()
};

UCLASS()
class NAUSEA_API UWeakpointDamageLogModifier : public UDamageLogModifierObject
{
	GENERATED_UCLASS_BODY()
};

UCLASS()
class NAUSEA_API UPartDestroyedFlatDamageLogModifier : public UDamageLogModifierObject
{
	GENERATED_UCLASS_BODY()
};

UCLASS()
class NAUSEA_API UPartDestroyedPercentDamageLogModifier : public UDamageLogModifierObject
{
	GENERATED_UCLASS_BODY()
};

UCLASS()
class NAUSEA_API UPartWeaknessDamageLogModifier : public UDamageLogModifierObject
{
	GENERATED_UCLASS_BODY()
};

UCLASS()
class NAUSEA_API UPartResistanceDamageLogModifier : public UDamageLogModifierObject
{
	GENERATED_UCLASS_BODY()
};

UCLASS()
class NAUSEA_API UWeaknessDamageLogModifier : public UDamageLogModifierObject
{
	GENERATED_UCLASS_BODY()
};

UCLASS()
class NAUSEA_API UResistanceDamageLogModifier : public UDamageLogModifierObject
{
	GENERATED_UCLASS_BODY()
};

UCLASS()
class NAUSEA_API UFriendlyFireScalingDamageLogModifier : public UDamageLogModifierObject
{
	GENERATED_UCLASS_BODY()
};