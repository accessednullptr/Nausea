// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#include "Player/PlayerClass/PlayerClassSkill.h"
#include "Net/UnrealNetwork.h"
#include "Engine/NetDriver.h"
#include "Internationalization/StringTableRegistry.h"
#include "GameFramework/Actor.h"
#include "Player/NauseaPlayerState.h"
#include "Player/PlayerClassComponent.h"
#include "Gameplay/StatusComponent.h"
#include "Weapon/Weapon.h"
#include "Weapon/FireMode.h"
#include "Weapon/FireMode/Ammo.h"
#include "Weapon/FireMode/LoadedAmmo.h"
#include "Gameplay/CoreDamageType.h"

UPlayerClassSkill::UPlayerClassSkill(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UPlayerClassSkill::Initialize(UPlayerClassComponent* OwningComponent)
{
	if (IsInitialized())
	{
		return;
	}

	Super::Initialize(OwningComponent);

	InitializeSkill(OwningComponent);
}

void UPlayerClassSkill::SetVariantAndIndex(EPlayerClassVariant Variant, int32 Index)
{
	PlayerSkillVariant = Variant;
	PlayerSkillIndex = Index;
}

void UPlayerClassSkill::InitializeSkill_Implementation(UPlayerClassComponent* PlayerClass)
{
	
}

FText UPlayerClassSkill::GetSkillName_Implementation() const
{
	return SkillName;
}

FText UPlayerClassSkill::GetSkillDescription_Implementation() const
{
	return FText::FromString("MISSING SKILL DESC");
}

bool UPlayerClassSkill::IsRelevantWeapon(const UWeapon* Weapon) const
{
	if (!Weapon)
	{
		return false;
	}

	return IsRelevantWeaponByDescriptors(Weapon->GetDescriptors());
}

bool UPlayerClassSkill::IsRelevantFireMode(const UFireMode* FireMode) const
{
	if (!FireMode || !FireMode->GetOwningWeapon())
	{
		return false;
	}

	if (!IsRelevantWeapon(FireMode->GetOwningWeapon()))
	{
		return false;
	}

	return IsRelevantFireModeByDescriptors(FireMode->GetDescriptors());
}

bool UPlayerClassSkill::IsRelevantAmmo(const UAmmo* Ammo) const
{
	if (!Ammo || !Ammo->GetOwningFireMode())
	{
		return false;
	}

	if (!IsRelevantFireMode(Ammo->GetOwningFireMode()))
	{
		return false;
	}

	return IsRelevantAmmoByDescriptors(Ammo->GetDescriptors());
}

bool UPlayerClassSkill::IsRelevantDamageType(const UCoreDamageType* DamageType) const
{
	if (!DamageType)
	{
		//If both filters are null, we're allowing all damage types anyways.
		if (DamageHitDescriptor == 0 && DamageElementalDescriptor == 0)
		{
			return true;
		}

		return false;
	}

	if (!IsRelevantHitDamageByDescriptors(DamageType->GetDamageHitDescriptors()))
	{
		return false;
	}

	if (!IsRelevantElementalDamageByDescriptors(DamageType->GetDamageElementalDescriptors()))
	{
		return false;
	}

	return true;
}

const UPlayerClassSkill* UPlayerClassSkill::GetPlayerClassSkillCDO() const
{
	const UPlayerClassComponent* PlayerClassComponent = GetPlayerClassComponent();

	if (!PlayerClassComponent)
	{
		return nullptr;
	}

	const UPlayerClassComponent* PlayerClassComponentCDO = PlayerClassComponent->GetClass()->GetDefaultObject<UPlayerClassComponent>();

	if (!PlayerClassComponentCDO)
	{
		return nullptr;
	}

	return PlayerClassComponentCDO->GetPlayerClassSkillByVariantAndIndex(PlayerSkillVariant, PlayerSkillIndex);
}

UPlayerClassSimplePassiveSkill::UPlayerClassSimplePassiveSkill(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{	
	SkillDescription = LOCTABLE("/Game/Localization/PlayerClassStringTable.PlayerClassStringTable", "SimplePassiveSkill_Description");
	ReductionSkillDescription = LOCTABLE("/Game/Localization/PlayerClassStringTable.PlayerClassStringTable", "SimplePassiveSkill_Description_Reduction");
}

inline bool ShouldSimplePassiveSkillUseReduction(ESimplePassiveSkill SimplePassiveSkill)
{
	switch (SimplePassiveSkill)
	{
	case ESimplePassiveSkill::EquipRate:
	case ESimplePassiveSkill::DamageTaken:
		return true;

	}
	return false;
}

void UPlayerClassSimplePassiveSkill::InitializeSkill_Implementation(UPlayerClassComponent* PlayerClass)
{
	Super::InitializeSkill_Implementation(PlayerClass);

	switch (PassiveSkill)
	{
	case ESimplePassiveSkill::EquipRate:
		PlayerClass->OnProcessEquipTime.AddUObject(this, &UPlayerClassSimplePassiveSkill::ProcessWeaponEvent);
		PlayerClass->OnProcessPutDownTime.AddUObject(this, &UPlayerClassSimplePassiveSkill::ProcessWeaponEvent);
		bPowerAsReduction = true;
		return;
	case ESimplePassiveSkill::FireRate:
		PlayerClass->OnProcessFireRate.AddUObject(this, &UPlayerClassSimplePassiveSkill::ProcessFireModeEvent);
		bPowerAsReduction = true;
		return;
	case ESimplePassiveSkill::Recoil:
		PlayerClass->OnProcessRecoil.AddUObject(this, &UPlayerClassSimplePassiveSkill::ProcessRecoilEvent);
		bPowerAsReduction = true;
		return;
	case ESimplePassiveSkill::AmmoCapacity:
		PlayerClass->OnProcessAmmoCapacity.AddUObject(this, &UPlayerClassSimplePassiveSkill::ProcessAmmoEvent);
		bPowerAsReduction = false;
		return;
	case ESimplePassiveSkill::LoadedAmmoCapacity:
		PlayerClass->OnProcessLoadedAmmoCapacity.AddUObject(this, &UPlayerClassSimplePassiveSkill::ProcessLoadedAmmoEvent);
		bPowerAsReduction = false;
		return;
	case ESimplePassiveSkill::ReloadRate:
		PlayerClass->OnProcessReloadRate.AddUObject(this, &UPlayerClassSimplePassiveSkill::ProcessLoadedAmmoEvent);
		bPowerAsReduction = true;
		return;
	case ESimplePassiveSkill::MovementSpeed:
		PlayerClass->OnProcessMovementSpeed.AddUObject(this, &UPlayerClassSimplePassiveSkill::ProcessCharacterEvent);
		return;
	case ESimplePassiveSkill::DamageTaken:
		PlayerClass->OnProcessDamageTaken.AddUObject(this, &UPlayerClassSimplePassiveSkill::ProcessDamageValueEvent);
		bPowerAsReduction = true;
		return;
	case ESimplePassiveSkill::DamageDealt:
		PlayerClass->OnProcessDamageDealt.AddUObject(this, &UPlayerClassSimplePassiveSkill::ProcessDamageValueEvent);
		bPowerAsReduction = false;
		return;
	case ESimplePassiveSkill::MaxHealth:
		PlayerClass->OnProcessOwnedActorMaxHealth.AddUObject(this, &UPlayerClassSimplePassiveSkill::ProcessStatValueEvent);
		return;
	case ESimplePassiveSkill::MaxArmour:
		PlayerClass->OnProcessOwnedActorMaxArmour.AddUObject(this, &UPlayerClassSimplePassiveSkill::ProcessStatValueEvent);
		return;
	}
}

FText UPlayerClassSimplePassiveSkill::GetSkillDescription_Implementation() const
{
	if (ShouldSimplePassiveSkillUseReduction(PassiveSkill))
	{
		return FText::FormatNamed(ReductionSkillDescription,
			TEXT("PassiveSkill"), GetNameOfSimplePassiveSkill(PassiveSkill),
			TEXT("Percent"), GetSkillPowerForSkillDescription());
	}

	return FText::FormatNamed(SkillDescription,
		TEXT("PassiveSkill"), GetNameOfSimplePassiveSkill(PassiveSkill),
		TEXT("Percent"), GetSkillPowerForSkillDescription());
}

float UPlayerClassSimplePassiveSkill::GetSkillPower() const
{
	return BasePower + PowerPerLevel;
}

FText UPlayerClassSimplePassiveSkill::GetSkillPowerForSkillDescription() const
{
	return FText::AsPercent(GetSkillPower());
}

void UPlayerClassSimplePassiveSkill::ApplySkillToValue(float& Value) const
{
	if (bPowerAsReduction)
	{
		Value *= FMath::Max(1.f - GetSkillPower(), 0.f);
	}
	else
	{
		Value *= 1.f + GetSkillPower();
	}
}

void UPlayerClassSimplePassiveSkill::ProcessWeaponEvent(const UWeapon* Weapon, float& Value)
{
	if (!IsRelevantWeapon(Weapon))
	{
		return;
	}

	ApplySkillToValue(Value);
}

void UPlayerClassSimplePassiveSkill::ProcessFireModeEvent(const UWeapon* Weapon, const UFireMode* FireMode, float& Value)
{
	if (!IsRelevantFireMode(FireMode))
	{
		return;
	}

	ApplySkillToValue(Value);
}

void UPlayerClassSimplePassiveSkill::ProcessRecoilEvent(const UWeapon* Weapon, const UFireMode* FireMode, float& RecoilX, float& RecoilY, float& Rate)
{
	if (!IsRelevantFireMode(FireMode))
	{
		return;
	}

	ApplySkillToValue(RecoilX);
	ApplySkillToValue(RecoilY);
	ApplySkillToValue(Rate);
}

void UPlayerClassSimplePassiveSkill::ProcessAmmoEvent(const UWeapon* Weapon, const UAmmo* Ammo, float& Value)
{
	if (!IsRelevantAmmo(Ammo))
	{
		return;
	}

	ApplySkillToValue(Value);
}

void UPlayerClassSimplePassiveSkill::ProcessLoadedAmmoEvent(const UWeapon* Weapon, const ULoadedAmmo* LoadedAmmo, float& Value)
{
	if (!IsRelevantAmmo(LoadedAmmo))
	{
		return;
	}

	ApplySkillToValue(Value);
}

void UPlayerClassSimplePassiveSkill::ProcessDamageValueEvent(UStatusComponent* Target, float& Value, const struct FDamageEvent& DamageEvent, ACorePlayerState* Instigator)
{
	if (!IsRelevantDamageType(Cast<UCoreDamageType>(DamageEvent.DamageTypeClass.GetDefaultObject())))
	{
		return;
	}

	const float InitialValue = Value;
	ApplySkillToValue(Value);

	if (InitialValue == Value || !Target || !Target->ShouldPerformDamageLog())
	{
		return;
	}

	TArray<UPlayerClassSkill*> CDOSkillList;
	switch (PlayerSkillVariant)
	{
	case EPlayerClassVariant::Invalid:
		ensure(false);
		break;
	case EPlayerClassVariant::Special:
		CDOSkillList = UPlayerClassComponent::GetCoreSkillListOfClass(GetPlayerClassComponent()->GetClass());
		break;
	default:
		CDOSkillList = UPlayerClassComponent::GetVariantSkillListOfClass(GetPlayerClassComponent()->GetClass(), PlayerSkillVariant);
		break;
	}

	if (!ensure(CDOSkillList.IsValidIndex(PlayerSkillIndex)))
	{
		return;
	}

	ensure(CDOSkillList[PlayerSkillIndex]->GetSkillName().IdenticalTo(GetSkillName()));

	Target->PushDamageLogModifier(FDamageLogEventModifier(CDOSkillList[PlayerSkillIndex], GetPlayerClassComponent()->GetOwningPlayerState(), Value / InitialValue));
}

void UPlayerClassSimplePassiveSkill::ProcessStatValueEvent(AActor* TargetActor, float& Value)
{
	ApplySkillToValue(Value);
}

void UPlayerClassSimplePassiveSkill::ProcessCharacterEvent(const ACoreCharacter* Character, float& Value)
{
	ApplySkillToValue(Value);
}

FText UPlayerClassSimplePassiveSkill::GetNameOfSimplePassiveSkill(ESimplePassiveSkill SimplePassiveSkill)
{
	return UPlayerClassHelpers::GetNameOfSimplePassiveSkill(SimplePassiveSkill);
}