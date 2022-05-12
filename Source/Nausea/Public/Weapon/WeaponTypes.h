#pragma once

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	Active, //Idle
	Inactive, //Down
	Equipping, //Equipping
	PuttingDown, //Putting Away
	Custom = 254, //Other - Used with UWeapon::CustomWeaponState.
	None = 255
};

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	Primary,
	Secondary,
	Tertiary,
	Quaternary,
	Quinary,
	MAX
};

const int32 MAXFIREMODES = int32(EFireMode::MAX);

UENUM(BlueprintType)
enum class EWeaponGroup : uint8
{
	Melee,
	Pistol,
	SMG,
	Rifle,
	Special,
	Utility,
	None
};

const int32 MAXWEAPONGROUP = int32(EWeaponGroup::None);

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EWeaponDescriptor : uint8
{
	None = 0 UMETA(Hidden),
	Melee = 1 << 0,
	Pistol = 1 << 1,
	Light = 1 << 2,
	Heavy = 1 << 3,
	Ranged = 1 << 4,
	Explosive = 1 << 5
};
ENUM_CLASS_FLAGS(EWeaponDescriptor);

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EFireModeDescriptor : uint8
{
	None = 0 UMETA(Hidden),
	Melee = 1 << 0,
	Ballistic = 1 << 1,
	Explosive = 1 << 2
};
ENUM_CLASS_FLAGS(EFireModeDescriptor);

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EAmmoDescriptor : uint8
{
	None = 0 UMETA(Hidden),
	Ballistic = 1 << 1,
	Rocket = 1 << 2,
	Explosive = 1 << 3
};
ENUM_CLASS_FLAGS(EAmmoDescriptor);