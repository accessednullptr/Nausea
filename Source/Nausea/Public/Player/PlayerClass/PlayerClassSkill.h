// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Weapon/WeaponTypes.h"
#include "Gameplay/StatusType.h"
#include "Player/PlayerClass/PlayerClassObject.h"
#include "Player/PlayerClass/PlayerClassTypes.h"
#include "Gameplay/DamageLogInterface.h"
#include "PlayerClassSkill.generated.h"

class UPlayerClassComponent;
class UWeapon;
class UFireMode;
class UAmmo;
class ULoadedAmmo;
class UCoreDamageType;

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, AutoExpandCategories = (Default))
class NAUSEA_API UPlayerClassSkill : public UPlayerClassObject
{
	GENERATED_UCLASS_BODY()

//~ Begin UPlayerClassObject Interface
public:
	virtual void Initialize(UPlayerClassComponent* OwningComponent) override;
//~ End UPlayerClassObject Interface

public:
	void SetVariantAndIndex(EPlayerClassVariant Variant, int32 Index);

	UFUNCTION(BlueprintNativeEvent, Category = Skill)
	void InitializeSkill(UPlayerClassComponent* PlayerClass);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Description)
	FText GetSkillName() const;
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Description)
	FText GetSkillDescription() const;
	UFUNCTION(BlueprintCallable, Category = Description)
	TSoftObjectPtr<UTexture2D> GetSkillIcon() const { return SkillIcon; }

	UFUNCTION(BlueprintCallable, Category = Skill)
	bool IsRelevantWeapon(const UWeapon* Weapon) const;
	UFUNCTION(BlueprintCallable, Category = Skill)
	bool IsRelevantFireMode(const UFireMode* FireMode) const;
	UFUNCTION(BlueprintCallable, Category = Skill)
	bool IsRelevantAmmo(const UAmmo* Ammo) const;
	UFUNCTION(BlueprintCallable, Category = Skill)
	bool IsRelevantDamageType(const UCoreDamageType* DamageType) const;

	UFUNCTION(BlueprintCallable, Category = Skill)
	FORCEINLINE bool IsRelevantWeaponByDescriptors(UPARAM(meta=(Bitmask,BitmaskEnum=EWeaponDescriptor)) int32 Flags) const
	{
		return WeaponDescriptor == 0 || (int32(WeaponDescriptor) & Flags) != 0;
	}

	UFUNCTION(BlueprintCallable, Category = Skill)
	FORCEINLINE bool IsRelevantFireModeByDescriptors(UPARAM(meta = (Bitmask, BitmaskEnum = EFireModeDescriptor)) int32 Flags) const
	{
		return FireModeDescriptor == 0 || (int32(FireModeDescriptor) & Flags) != 0;
	}

	UFUNCTION(BlueprintCallable, Category = Skill)
	FORCEINLINE bool IsRelevantAmmoByDescriptors(UPARAM(meta=(Bitmask,BitmaskEnum=EAmmoDescriptor)) int32 Flags) const
	{
		return AmmoDescriptor == 0 || (int32(AmmoDescriptor) & Flags) != 0;
	}

	UFUNCTION(BlueprintCallable, Category = Skill)
	FORCEINLINE bool IsRelevantHitDamageByDescriptors(UPARAM(meta=(Bitmask,BitmaskEnum=EAmmoDescriptor)) int32 Flags) const
	{
		return DamageHitDescriptor == 0 || (int32(DamageHitDescriptor) & Flags) != 0;
	}

	UFUNCTION(BlueprintCallable, Category = Skill)
	FORCEINLINE bool IsRelevantElementalDamageByDescriptors(UPARAM(meta=(Bitmask,BitmaskEnum=EAmmoDescriptor)) int32 Flags) const
	{
		return DamageElementalDescriptor == 0 || (int32(DamageElementalDescriptor) & Flags) != 0;
	}

	const UPlayerClassSkill* GetPlayerClassSkillCDO() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = Description)
	FText SkillName = FText();
	UPROPERTY(EditDefaultsOnly, Category = Description)
	TSoftObjectPtr<UTexture2D> SkillIcon = nullptr;
	
	//Used to filter what weapons this skill applies to. If empty, UPlayerClassSkill::IsRelevantWeaponByDescriptors will always return true.
	UPROPERTY(EditDefaultsOnly, Category = Filters, meta = (Bitmask, BitmaskEnum = EWeaponDescriptor))
	uint8 WeaponDescriptor = 0;

	//Used to filter what firemodes this skill applies to. If empty, UPlayerClassSkill::IsRelevantFireModeByDescriptors will always return true.
	UPROPERTY(EditDefaultsOnly, Category = Filters, meta = (Bitmask, BitmaskEnum = EFireModeDescriptor))
	uint8 FireModeDescriptor = 0;

	//Used to filter what ammo types this skill applies to. If empty, UPlayerClassSkill::IsRelevanIsRelevantAmmoByDescriptorstWeaponByDescriptors will always return true.
	UPROPERTY(EditDefaultsOnly, Category = Filters, meta = (Bitmask, BitmaskEnum = EAmmoDescriptor))
	uint8 AmmoDescriptor = 0;

	//Used to filter what damage hit types this skill applies to. If empty, UPlayerClassSkill::IsRelevantHitDamageByDescriptors will always return true.
	UPROPERTY(EditDefaultsOnly, Category = Filters, meta = (Bitmask, BitmaskEnum = EDamageHitDescriptor))
	uint8 DamageHitDescriptor = 0;

	//Used to filter what damage hit types this skill applies to. If empty, UPlayerClassSkill::IsRelevantElementalDamageByDescriptors will always return true.
	UPROPERTY(EditDefaultsOnly, Category = Filters, meta = (Bitmask, BitmaskEnum = EDamageElementalDescriptor))
	uint8 DamageElementalDescriptor = 0;

	//Data cached on initialization. Used for fast CDO lookup.
	UPROPERTY()
	EPlayerClassVariant PlayerSkillVariant = EPlayerClassVariant::Invalid;
	UPROPERTY()
	int32 PlayerSkillIndex = -1;
};

UCLASS(EditInlineNew, AutoExpandCategories = (Default, PassiveSkill), HideFunctions = (K2_GetDamageLogInstigatorName))
class NAUSEA_API UPlayerClassSimplePassiveSkill : public UPlayerClassSkill, public IDamageLogInterface
{
	GENERATED_UCLASS_BODY()

//~ Begin UPlayerClassSkill Interface
public:
	virtual void InitializeSkill_Implementation(UPlayerClassComponent* PlayerClass) override;
	virtual FText GetSkillDescription_Implementation() const override;
//~ End UPlayerClassSkill Interface

//~ Begin IDamageLogInterface Interface
public:
	virtual FText GetDamageLogInstigatorName() const override { return GetSkillName(); }
//~ End IDamageLogInterface Interface

	UFUNCTION(BlueprintCallable, Category = "Simple Passive Skill")
	float GetSkillPower() const;

	UFUNCTION(BlueprintCallable, Category = "Simple Passive Skill")
	FText GetSkillPowerForSkillDescription() const;

protected:
	void ApplySkillToValue(float& Value) const;

	UFUNCTION()
	void ProcessWeaponEvent(const UWeapon* Weapon, float& Value);

	UFUNCTION()
	void ProcessFireModeEvent(const UWeapon* Weapon, const UFireMode* FireMode, float& Value);

	UFUNCTION()
	void ProcessRecoilEvent(const UWeapon* Weapon, const UFireMode* FireMode, float& RecoilX, float& RecoilY, float& Rate);

	UFUNCTION()
	void ProcessAmmoEvent(const UWeapon* Weapon, const UAmmo* Ammo, float& Value);

	UFUNCTION()
	void ProcessLoadedAmmoEvent(const UWeapon* Weapon, const ULoadedAmmo* LoadedAmmo, float& Value);

	UFUNCTION()
	void ProcessDamageValueEvent(UStatusComponent* Target, float& Value, const struct FDamageEvent& DamageEvent, ACorePlayerState* Instigator);

	UFUNCTION()
	void ProcessStatValueEvent(AActor* TargetActor, float& Value);

	UFUNCTION()
	void ProcessCharacterEvent(const ACoreCharacter* Character, float& Value);

protected:
	UPROPERTY(EditDefaultsOnly, Category = Description)
	FText SkillDescription;
	//Used as an alternative to SkillDescription when the simple passive skill is described as a reduction (determined by ESimplePassiveSkill).
	UPROPERTY(EditDefaultsOnly, Category = Description)
	FText ReductionSkillDescription;

	UPROPERTY(EditDefaultsOnly, Category = "Simple Passive Skill")
	ESimplePassiveSkill PassiveSkill = ESimplePassiveSkill::Invalid;

	UPROPERTY(EditDefaultsOnly, Category = "Simple Passive Skill")
	float BasePower = 0.f;
	UPROPERTY(EditDefaultsOnly, Category = "Simple Passive Skill")
	float PowerPerLevel = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Simple Passive Skill")
	class UCurveFloat* CurveBonus = nullptr;

	UPROPERTY()
	bool bPowerAsReduction = false;

public:
	UFUNCTION(BlueprintCallable, Category = "Simple Passive Skill")
	static FText GetNameOfSimplePassiveSkill(ESimplePassiveSkill SimplePassiveSkill);
};
