// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Gameplay/StatusEffect/StatusEffectShield.h"
#include "NauseaNetDefines.h"
#include "Player/CorePlayerState.h"
#include "Gameplay/StatusComponent.h"

UStatusEffectShield::UStatusEffectShield(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bBindProcessDamage = true;
	ShieldDamageLogModifierClass = UStatusEffectShieldDamageLogModifier::StaticClass();
}

void UStatusEffectShield::OnActivated(EStatusBeginType BeginType)
{
	Super::OnActivated(BeginType);
	CurrentShieldAmount = ShieldAmount;

	CurrentPower = 1.f;
	OnRep_CurrentPower();
}

float UStatusEffectShield::GetDurationAtCurrentPower() const
{
	return -1.f;
}

void UStatusEffectShield::ProcessDamage(UStatusComponent* Component, float& Damage, const struct FDamageEvent& DamageEvent, ACorePlayerState* Instigator)
{
	if (!Component || Damage <= 0.f)
	{
		return;
	}

	Super::ProcessDamage(Component, Damage, DamageEvent, Instigator);

	const float ShieldAbsorb = FMath::Min(Damage, CurrentShieldAmount);

	CurrentShieldAmount -= Damage;

	const float CachedCurrentPower = CurrentPower;
	CurrentPower = CurrentShieldAmount / ShieldAmount;

	if (CachedCurrentPower != CurrentPower)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(UStatusEffectBasic, CurrentPower, this);
	}

	OnRep_CurrentPower();

	Component->PushDamageLogModifier(FDamageLogEventModifier(ShieldDamageLogModifierClass, StatusEffectInsitgator, (Damage - ShieldAbsorb) / Damage));

	Damage = FMath::Max(Damage - ShieldAbsorb, 0.f);

	if (CurrentPower <= 0.f)
	{
		OnDeactivated(EStatusEndType::Expired);
	}
}

void UStatusEffectShield::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsAuthority() && CurrentPower <= 0.f)
	{
		OnDeactivated(EStatusEndType::Expired);
	}
}

UStatusEffectShieldDamageLogModifier::UStatusEffectShieldDamageLogModifier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstigatorName = LOCTABLE("/Game/Localization/DamageLogStringTable.DamageLogStringTable", "Modification_Shield");
}