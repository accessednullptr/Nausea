// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Gameplay/DamageLogModifier/DamageLogModifierObject.h"
#include "Internationalization/StringTableRegistry.h"

UDamageLogModifierObject::UDamageLogModifierObject(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UArmorDamageLogModifier::UArmorDamageLogModifier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstigatorName = LOCTABLE("/Game/Localization/DamageLogStringTable.DamageLogStringTable", "Modification_Armor");
}

UWeakpointDamageLogModifier::UWeakpointDamageLogModifier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstigatorName = LOCTABLE("/Game/Localization/DamageLogStringTable.DamageLogStringTable", "Modification_Weakpoint");
}

UPartDestroyedFlatDamageLogModifier::UPartDestroyedFlatDamageLogModifier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstigatorName = LOCTABLE("/Game/Localization/DamageLogStringTable.DamageLogStringTable", "Modification_PartDestroyedFlatDamage");
}

UPartDestroyedPercentDamageLogModifier::UPartDestroyedPercentDamageLogModifier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstigatorName = LOCTABLE("/Game/Localization/DamageLogStringTable.DamageLogStringTable", "Modification_PartDestroyedPercentDamage");
}

UPartWeaknessDamageLogModifier::UPartWeaknessDamageLogModifier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstigatorName = LOCTABLE("/Game/Localization/DamageLogStringTable.DamageLogStringTable", "Modification_PartDamageWeakness");
}

UPartResistanceDamageLogModifier::UPartResistanceDamageLogModifier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstigatorName = LOCTABLE("/Game/Localization/DamageLogStringTable.DamageLogStringTable", "Modification_PartDamageResistance");
}

UWeaknessDamageLogModifier::UWeaknessDamageLogModifier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstigatorName = LOCTABLE("/Game/Localization/DamageLogStringTable.DamageLogStringTable", "Modification_DamageWeakness");
}

UResistanceDamageLogModifier::UResistanceDamageLogModifier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstigatorName = LOCTABLE("/Game/Localization/DamageLogStringTable.DamageLogStringTable", "Modification_DamageResistance");
}

UFriendlyFireScalingDamageLogModifier::UFriendlyFireScalingDamageLogModifier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstigatorName = LOCTABLE("/Game/Localization/DamageLogStringTable.DamageLogStringTable", "Modification_FriendlyFireScaling");
}