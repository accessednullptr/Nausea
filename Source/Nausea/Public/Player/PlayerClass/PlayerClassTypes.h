#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Internationalization/StringTableRegistry.h"
#include "PlayerClassTypes.generated.h"

UENUM(BlueprintType)
enum class EPlayerClassVariant : uint8
{
	Invalid,
	Primary,
	Alternative,
	Special,
	Core = 254,
	MAX = 255
};

UENUM(BlueprintType)
enum class EPlayerClassSelectionResponse : uint8
{
	//Failed Responses
	Invalid,
	NotAvailable,
	//Success Responses
	Success,
	Pending,
	AlreadySelected
};

UENUM(BlueprintType)
enum class EInventorySelectionResponse : uint8
{
	//Failed Responses
	Invalid,
	NotAvailable,
	//Success Responses
	Success,
	Pending,
	AlreadySelected,
	SuccessWithChanges //Should only happen if the client is desynced and has sent a inventory selection that the server found was invalid. Server will send a corrected loadout if so.
};

UENUM(BlueprintType)
enum class ESimplePassiveSkill : uint8
{
	Invalid,
	EquipRate,
	FireRate,
	Recoil,
	ReloadRate,
	AmmoCapacity,
	LoadedAmmoCapacity,
	MovementSpeed,
	DamageTaken,
	DamageDealt,
	StatusPowerTaken,
	StatusPowerDealt,
	MaxHealth,
	MaxArmour
};


static FText SimplePassiveSkillFireRate = LOCTABLE("/Game/Localization/PlayerClassStringTable.PlayerClassStringTable", "SimplePassiveSkill_Type_FireRate");
static FText SimplePassiveSkillReloadRate = LOCTABLE("/Game/Localization/PlayerClassStringTable.PlayerClassStringTable", "SimplePassiveSkill_Type_ReloadRate");
static FText SimplePassiveSkillAmmoCapacity = LOCTABLE("/Game/Localization/PlayerClassStringTable.PlayerClassStringTable", "SimplePassiveSkill_Type_AmmoCapacity");
static FText SimplePassiveSkillLoadedAmmoCapacity = LOCTABLE("/Game/Localization/PlayerClassStringTable.PlayerClassStringTable", "SimplePassiveSkill_Type_LoadedAmmoCapacity");
static FText SimplePassiveSkillEquipRate = LOCTABLE("/Game/Localization/PlayerClassStringTable.PlayerClassStringTable", "SimplePassiveSkill_Type_EquipRate");
static FText SimplePassiveSkillMovementSpeed = LOCTABLE("/Game/Localization/PlayerClassStringTable.PlayerClassStringTable", "SimplePassiveSkill_Type_MovementSpeed");
static FText SimplePassiveSkillDamageTaken = LOCTABLE("/Game/Localization/PlayerClassStringTable.PlayerClassStringTable", "SimplePassiveSkill_Type_DamageTaken");
static FText SimplePassiveSkillDamageDealt = LOCTABLE("/Game/Localization/PlayerClassStringTable.PlayerClassStringTable", "SimplePassiveSkill_Type_DamageDealt");
static FText SimplePassiveSkillStatusPowerTaken = LOCTABLE("/Game/Localization/PlayerClassStringTable.PlayerClassStringTable", "SimplePassiveSkill_Type_StatusPowerTaken");
static FText SimplePassiveSkillStatusPowerDealt = LOCTABLE("/Game/Localization/PlayerClassStringTable.PlayerClassStringTable", "SimplePassiveSkill_Type_StatusPowerDealt");
static FText SimplePassiveSkillMaxHealth = LOCTABLE("/Game/Localization/PlayerClassStringTable.PlayerClassStringTable", "SimplePassiveSkill_Type_MaxHealth");
static FText SimplePassiveSkillMaxArmour = LOCTABLE("/Game/Localization/PlayerClassStringTable.PlayerClassStringTable", "SimplePassiveSkill_Type_MaxArmour");
static FText SimplePassiveSkillInvalid = LOCTABLE("/Game/Localization/PlayerClassStringTable.PlayerClassStringTable", "SimplePassiveSkill_Type_Invalid");

UCLASS()
class NAUSEA_API UPlayerClassHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	static EPlayerClassVariant GetOppositeVariant(EPlayerClassVariant Variant)
	{
		switch (Variant)
		{
		case EPlayerClassVariant::Primary:
			return EPlayerClassVariant::Alternative;
		case EPlayerClassVariant::Alternative:
			return EPlayerClassVariant::Primary;
		}

		return EPlayerClassVariant::Invalid;
	}

	static FText GetNameOfSimplePassiveSkill(ESimplePassiveSkill SimplePassiveSkill)
	{
		switch (SimplePassiveSkill)
		{
		case ESimplePassiveSkill::FireRate:
			return SimplePassiveSkillFireRate;
		case ESimplePassiveSkill::ReloadRate:
			return SimplePassiveSkillReloadRate;
		case ESimplePassiveSkill::AmmoCapacity:
			return SimplePassiveSkillAmmoCapacity;
		case ESimplePassiveSkill::LoadedAmmoCapacity:
			return SimplePassiveSkillLoadedAmmoCapacity;
		case ESimplePassiveSkill::EquipRate:
			return SimplePassiveSkillEquipRate;
		case ESimplePassiveSkill::MovementSpeed:
			return SimplePassiveSkillMovementSpeed;
		case ESimplePassiveSkill::DamageTaken:
			return SimplePassiveSkillDamageTaken;
		case ESimplePassiveSkill::DamageDealt:
			return SimplePassiveSkillDamageDealt;
		case ESimplePassiveSkill::StatusPowerTaken:
			return SimplePassiveSkillStatusPowerTaken;
		case ESimplePassiveSkill::StatusPowerDealt:
			return SimplePassiveSkillStatusPowerDealt;
		case ESimplePassiveSkill::MaxHealth:
			return SimplePassiveSkillMaxHealth;
		case ESimplePassiveSkill::MaxArmour:
			return SimplePassiveSkillMaxArmour;

		default: //case ESimplePassiveSkill::Invalid:
			return SimplePassiveSkillInvalid;
		}

		return SimplePassiveSkillInvalid;
	}
};