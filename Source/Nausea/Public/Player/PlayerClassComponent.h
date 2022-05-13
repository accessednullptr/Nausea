// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/PlayerClass/PlayerClassTypes.h"
#include "Player/PlayerOwnershipInterface.h"
#include "System/ReplicatedObjectInterface.h"
#include "PlayerClassComponent.generated.h"

class ACorePlayerState;

class UPlayerClassSkill;
class UPlayerClassExperienceSource;

class ACoreCharacter;
class UStatusComponent;
class UInventory;
class UWeapon;
class UFireMode;
class UAmmo;
class ULoadedAmmo;

USTRUCT(BlueprintType)
struct FSkillEntry
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FString SkillName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced)
	UPlayerClassSkill* Skill = nullptr;
};

USTRUCT(BlueprintType)
struct FExperienceSourceEntry
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly)
	FString SourceName;

	UPROPERTY(EditDefaultsOnly, Instanced)
	UPlayerClassExperienceSource* ExperienceSource = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLevelChangedSignature, UPlayerClassComponent*, PlayerClass, int32, Level);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLevelProgressChangedSignature, UPlayerClassComponent*, PlayerClass, float, Progress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSkillListUpdatedSignature, UPlayerClassComponent*, PlayerClass, const TArray<UPlayerClassSkill*>&, SkillList);

UCLASS(Blueprintable, BlueprintType, HideCategories = (ComponentTick, Collision, Tags, Variable, Activation, ComponentReplication, Cooking))
class NAUSEA_API UPlayerClassComponent : public UActorComponent, public IPlayerOwnershipInterface, public IReplicatedObjectInterface
{
	GENERATED_UCLASS_BODY()
		
//~ Begin UObject Interface	
public:
	virtual void Serialize(FArchive& Ar) override;
protected:
	virtual void PostNetReceive() override;
//~ Begin UObject Interface

//~ Begin UActorComponent Interface
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
public:
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
//~ End UActorComponent Interface

//~ Begin IPlayerOwnershipInterface Interface.
public:
	virtual ACorePlayerState* GetOwningPlayerState() const override;
	virtual AController* GetOwningController() const override;
	virtual APawn* GetOwningPawn() const override;
//~ End IPlayerOwnershipInterface Interface.

public:
	UFUNCTION(BlueprintCallable, Category = Class)
	FText GetTitle() const { return PlayerClassTitle; }

	UFUNCTION(BlueprintCallable, Category = Class)
	FText GetDescription() const { return PlayerClassDescription; }

	UFUNCTION(BlueprintCallable, Category = Class)
	const FLinearColor& GetPlayerClassColour() const;

	UFUNCTION(BlueprintCallable, Category = Class)
	int32 GetLevel() const { return Level; }
	UFUNCTION(BlueprintCallable, Category = Class)
	int32 GetMaxLevel() const { return MaxLevel; }

	UFUNCTION(BlueprintCallable, Category = Class)
	int32 SetLevel(int32 InLevel);

	UFUNCTION(BlueprintCallable, Category = Class)
	float SetExperiencePercent(float InExperienceProgress);

	UFUNCTION(BlueprintCallable, Category = Class)
	EPlayerClassVariant GetVariant() const { return Variant; }
	UFUNCTION()
	void SetVariant(EPlayerClassVariant InVariant);

	UFUNCTION(BlueprintCallable, Category = Class)
	float GetPercentExperience() const { return ExperiencePercent; }

	UCurveFloat* GetExperienceCurve() const { return ExperienceCurve; }

	UFUNCTION(BlueprintCallable, Category = Class)
	const TArray<UPlayerClassSkill*>& GetCoreSkillList() const { return CoreSkillList; }
	UFUNCTION(BlueprintCallable, Category = Class)
	const TArray<UPlayerClassSkill*>& GetPrimaryVariantSkillList() const { return PrimarySkillList; }
	UFUNCTION(BlueprintCallable, Category = Class)
	const TArray<UPlayerClassSkill*>& GetAlternativeVariantSkillList() const { return AlternativeSkillList; }

	UFUNCTION(BlueprintCallable, Category = Class)
	TArray<UPlayerClassSkill*> GetActiveSkillList() const;

	const UPlayerClassSkill* GetPlayerClassSkillByVariantAndIndex(EPlayerClassVariant SkillVariant, int32 SkillIndex) const;

	UFUNCTION(BlueprintCallable, Category = Class)
	TArray<UPlayerClassExperienceSource*> GetExperienceSourceList() const;

	UFUNCTION(BlueprintCallable, Category = Class)
	TArray<TSoftClassPtr<UInventory>> GetDefaultInventoryList() const;

	UFUNCTION(BlueprintCallable, Category = Class)
	void ProcessInitialInventoryList(TArray<TSubclassOf<UInventory>>& InventoryClassList) const;

protected:
	UFUNCTION()
	void OnRep_Level();

	UFUNCTION()
	void OnRep_ExperiencePercent();

	UFUNCTION()
	void OnRep_Variant();

	UFUNCTION()
	void InitializePlayerClass();

	UFUNCTION()
	void OnReceiveExperienceUpdate(UPlayerStatisticsComponent* PlayerStatistics, TSubclassOf<UPlayerClassComponent> TargetPlayerClass, EPlayerClassVariant TargetVariant, uint64 Delta);

public:
	UPROPERTY(BlueprintAssignable, Category = PlayerClass)
	FLevelChangedSignature OnLevelChanged;
	UPROPERTY(BlueprintAssignable, Category = PlayerClass)
	FLevelProgressChangedSignature OnLevelProgressChanged;

//Player class bonus hooks.
public:
	DECLARE_EVENT_TwoParams(UPlayerClassComponent, FWeaponValueModifierSignature, const UWeapon*, float&)
	FWeaponValueModifierSignature OnProcessEquipTime;
	FWeaponValueModifierSignature OnProcessPutDownTime;

	DECLARE_EVENT_ThreeParams(UPlayerClassComponent, FWeaponFireModeValueModifierSignature, const UWeapon*, const UFireMode*, float&)
	FWeaponFireModeValueModifierSignature OnProcessFireRate;

	//float RecoilX, float RecoilY, float RecoilDuration
	DECLARE_EVENT_FiveParams(UPlayerClassComponent, FWeaponRecoilValueModifierSignature, const UWeapon*, const UFireMode*, float&, float&, float&)
	FWeaponRecoilValueModifierSignature OnProcessRecoil;

	DECLARE_EVENT_ThreeParams(UPlayerClassComponent, FWeaponAmmoValueModifierSignature, const UWeapon*, const UAmmo*, float&)
	FWeaponAmmoValueModifierSignature OnProcessAmmoCapacity;

	DECLARE_EVENT_ThreeParams(UPlayerClassComponent, FWeaponLoadedAmmoValueModifierSignature, const UWeapon*, const ULoadedAmmo*, float&)
	FWeaponLoadedAmmoValueModifierSignature OnProcessLoadedAmmoCapacity;
	FWeaponLoadedAmmoValueModifierSignature OnProcessReloadRate;

	DECLARE_EVENT_FourParams(UPlayerClassComponent, FWeaponDamageValueModifierSignature, UStatusComponent*, float&, const struct FDamageEvent&, ACorePlayerState*)
	FWeaponDamageValueModifierSignature OnProcessDamageDealt;
	FWeaponDamageValueModifierSignature OnProcessDamageTaken;

	DECLARE_EVENT_TwoParams(UPlayerClassComponent, FPlayerBasicStatValueModifierSignature, AActor*, float&)
	FPlayerBasicStatValueModifierSignature OnProcessOwnedActorMaxHealth;
	FPlayerBasicStatValueModifierSignature OnProcessOwnedActorMaxArmour;

	//float ArmourAbsorption, float ArmourDecay
	DECLARE_EVENT_FiveParams(UPlayerClassComponent, FArmourDamageModifierSignature, float&, float&, const struct FDamageEvent&, AController*, AActor*)
	FArmourDamageModifierSignature OnProcessArmourDamage;

	DECLARE_EVENT_TwoParams(UPlayerClassComponent, FMovementSpeedModifierSignature, const ACoreCharacter*, float&)
	FMovementSpeedModifierSignature OnProcessMovementSpeed;

protected:
	UPROPERTY(EditDefaultsOnly, Category = Class, meta=(DisplayName="Title",ScriptName="Title"))
	FText PlayerClassTitle;
	UPROPERTY(EditDefaultsOnly, Category = Class, meta=(DisplayName="Description",ScriptName="Description"))
	FText PlayerClassDescription;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Icon")
	TSoftObjectPtr<UTexture2D> PlayerClassIcon;
	UPROPERTY(EditDefaultsOnly, Category = "UI|Icon")
	TSoftObjectPtr<UTexture2D> PlayerClassIconLarge;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Colour", meta=(DisplayName="Core Colour",ScriptName="CoreColour"))
	FLinearColor PlayerClassCoreColour;
	UPROPERTY(EditDefaultsOnly, Category = "UI|Colour", meta=(DisplayName="Primary Colour",ScriptName="PrimaryColour"))
	FLinearColor PlayerClassColourPrimaryVariant;
	UPROPERTY(EditDefaultsOnly, Category = "UI|Colour", meta=(DisplayName="Alternative Colour",ScriptName="AlternativeColour"))
	FLinearColor PlayerClassColourAlternativeVariant;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Backplate", meta=(DisplayName="Basic Primary Backplate",ScriptName="BasicPrimaryBackplate"))
	UMaterialInterface* PlayerClassPrimaryBackplate = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "UI|Backplate", meta=(DisplayName="Basic Alternative Backplate",ScriptName="BasicAlternativeBackplate"))
	UMaterialInterface* PlayerClassAlternativeBackplate = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Backplate", meta=(DisplayName="Hexagon Primary Backplate",ScriptName="HexagonPrimaryBackplate"))
	UMaterialInterface* PlayerClassPrimaryHexagonBackplate = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "UI|Backplate", meta=(DisplayName="Hexagon Alternative Backplate",ScriptName="HexagonAlternativeBackplate"))
	UMaterialInterface* PlayerClassAlternativeHexagonBackplate = nullptr;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_Variant)
	EPlayerClassVariant Variant = EPlayerClassVariant::Invalid;
	UPROPERTY(Transient)
	bool bHasReceivedVariant = false;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Core", meta=(DisplayName="Tier 1 Core Skill"))
	UPlayerClassSkill* Tier1CoreSkill;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Core", meta=(DisplayName="Tier 2 Core Skill"))
	UPlayerClassSkill* Tier2CoreSkill;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Core", meta=(DisplayName="Tier 3 Core Skill"))
	UPlayerClassSkill* Tier3CoreSkill;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Core", meta=(DisplayName="Tier 4 Core Skill"))
	UPlayerClassSkill* Tier4CoreSkill;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Primary Variant", meta=(DisplayName="Tier 1 Primary Skill"))
	UPlayerClassSkill* Tier1PrimarySkill;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Primary Variant", meta=(DisplayName="Tier 2 Primary Skill"))
	UPlayerClassSkill* Tier2PrimarySkill;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Primary Variant", meta=(DisplayName="Tier 3 Primary Skill"))
	UPlayerClassSkill* Tier3PrimarySkill;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Primary Variant", meta=(DisplayName="Tier 4 Primary Skill"))
	UPlayerClassSkill* Tier4PrimarySkill;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Alternative Variant", meta=(DisplayName="Tier 1 Alternative Skill"))
	UPlayerClassSkill* Tier1AlternativeSkill;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Alternative Variant", meta=(DisplayName="Tier 2 Alternative Skill"))
	UPlayerClassSkill* Tier2AlternativeSkill;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Alternative Variant", meta=(DisplayName="Tier 3 Alternative Skill"))
	UPlayerClassSkill* Tier3AlternativeSkill;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Alternative Variant", meta=(DisplayName="Tier 4 Alternative Skill"))
	UPlayerClassSkill* Tier4AlternativeSkill;

	UPROPERTY(EditDefaultsOnly, Category = "Core")
	TArray<TSoftClassPtr<UInventory>> CoreDefaultInventory;
	UPROPERTY(EditDefaultsOnly, Category = "Primary Variant")
	TArray<TSoftClassPtr<UInventory>> PrimaryDefaultInventory;
	UPROPERTY(EditDefaultsOnly, Category = "Alternative Variant")
	TArray<TSoftClassPtr<UInventory>> AlternateDefaultInventory;

	UPROPERTY()
	TArray<UPlayerClassSkill*> CoreSkillList;
	UPROPERTY()
	TArray<UPlayerClassSkill*> PrimarySkillList;
	UPROPERTY()
	TArray<UPlayerClassSkill*> AlternativeSkillList;

	UPROPERTY(EditDefaultsOnly, meta=(TitleProperty = "SourceName"), Category = Experience)
	TArray<FExperienceSourceEntry> ExperienceSourceList;

	UPROPERTY(EditDefaultsOnly, Category = Experience)
	UCurveFloat* ExperienceCurve = nullptr;

private:
	UPROPERTY(Transient, ReplicatedUsing = OnRep_Level)
	int32 Level = -1;

	UPROPERTY(EditDefaultsOnly, Category = Class)
	int32 MaxLevel = 10;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_ExperiencePercent)
	float ExperiencePercent = 0.f;

	UPROPERTY()
	ACoreCharacter* LastPossessedCharacter = nullptr;

	UPROPERTY(Transient)
	ACorePlayerState* OwningPlayerState = nullptr;

	UPROPERTY(Transient)
	FTimerHandle ExperienceUpdateThrottleTimer;
	UPROPERTY(Transient)
	bool bPendingExperienceUpdate = false;

public:
	UFUNCTION(BlueprintCallable, Category = PlayerClass)
	static TArray<UPlayerClassSkill*> GetCoreSkillListOfClass(TSubclassOf<UPlayerClassComponent> PlayerClass);

	UFUNCTION(BlueprintCallable, Category = PlayerClass)
	static TArray<UPlayerClassSkill*> GetVariantSkillListOfClass(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant PlayerClassVariant);
	
	UFUNCTION(BlueprintCallable, Category = Class)
	static TArray<UPlayerClassSkill*> GetActiveSkillListListOfClass(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant PlayerClassVariant);

	UFUNCTION(BlueprintCallable, Category = PlayerClass)
	static FText GetPlayerClassTitle(TSubclassOf<UPlayerClassComponent> PlayerClass);
	UFUNCTION(BlueprintCallable, Category = PlayerClass)
	static FText GetVariantName(EPlayerClassVariant PlayerClassVariant);

	UFUNCTION(BlueprintCallable, Category = PlayerClass)
	static FText GetPlayerClassDescription(TSubclassOf<UPlayerClassComponent> PlayerClass);

	UFUNCTION(BlueprintCallable, Category = PlayerClass)
	static TSoftObjectPtr<UTexture2D> GetPlayerClassIcon(TSubclassOf<UPlayerClassComponent> PlayerClass);
	UFUNCTION(BlueprintCallable, Category = PlayerClass)
	static TSoftObjectPtr<UTexture2D> GetPlayerClassIconLarge(TSubclassOf<UPlayerClassComponent> PlayerClass);
	UFUNCTION(BlueprintCallable, Category = PlayerClass)
	static const FLinearColor& GetPlayerClassCoreColour(TSubclassOf<UPlayerClassComponent> PlayerClass);
	UFUNCTION(BlueprintCallable, Category = PlayerClass)
	static const FLinearColor& GetPlayerClassVariantColour(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant PlayerClassVariant);

	UFUNCTION(BlueprintCallable, Category = PlayerClass, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static UMaterialInterface* GetPlayerClassBackplateMaterial(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant PlayerClassVariant);
	UFUNCTION(BlueprintCallable, Category = PlayerClass, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static UMaterialInterface* GetPlayerClassHexagonBackplateMaterial(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant PlayerClassVariant);

	UFUNCTION(BlueprintCallable, Category = PlayerClass, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static TArray<TSoftClassPtr<UInventory>> GetDefaultInventoryListFromClass(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant PlayerClassVariant);

	UFUNCTION(BlueprintCallable, Category = PlayerClass)
	static int32 GetPlayerClassLevel(TScriptInterface<IPlayerOwnershipInterface> PlayerOwnedInterface, TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant PlayerClassVariant);

	UFUNCTION(BlueprintCallable, Category = PlayerClass)
	static void GetPlayerClassLevelInfo(TScriptInterface<IPlayerOwnershipInterface> PlayerOwnedInterface, TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant PlayerClassVariant, int32& PlayerClassLevel, int64& ExperienceProgress, int64& ExperienceRequirement, float& PercentProgress);
};