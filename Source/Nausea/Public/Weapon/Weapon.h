// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "AI/CoreAITypes.h"
#include "WeaponTypes.h"
#include "Weapon/Inventory.h"
#include "Gameplay/DamageLogInterface.h"
#include "System/ReplicatedObjectInterface.h"
#include "Weapon.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogWeapon, Warning, All);

class UFireMode;
class UAmmo;
class USkeletalMesh;
class USkeletalMeshComponent;
class UTexture;
class UUserWidget;
class UAnimationObject;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FWeaponStateChangedSignature, UWeapon*, Weapon, EWeaponState, State, EWeaponState, PreviousState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWeaponEquipSignature, UWeapon*, Weapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWeaponPutDownSignature, UWeapon*, Weapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWeaponFireCompleteSignature, UWeapon*, Weapon, UFireMode*, FireMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWeaponReloadCompleteSignature, UWeapon*, Weapon, UAmmo*, Ammo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWeaponAwaitingPutDown, UWeapon*, Weapon);

/**
 * 
 */
UCLASS(HideFunctions = (K2_GetDamageLogInstigatorName))
class NAUSEA_API UWeapon : public UInventory, public IDamageLogInterface, public IReplicatedObjectInterface
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UObject Interface
public:
	virtual void Serialize(FArchive& Ar) override;
//~ End UObject Interface

//~ Begin UActorComponent Interface.
protected:
	virtual void BeginPlay() override;
public:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

//Disable all activation-related functionality.
private:
	virtual void Activate(bool bReset = false) override {}
	virtual void Deactivate() override {}
	virtual void SetActive(bool bNewActive, bool bReset = false) override {}
	virtual void ToggleActive() override {}
public:
	virtual bool IsActiveWeapon() const { return WeaponState == EWeaponState::Active; }
//~ End UActorComponent Interface.

//~ Begin UInventory Interface.
public:
	virtual float GetMovementSpeedModifier() const override { return !IsInactive() ? EquippedMovementSpeedModifier : Super::GetMovementSpeedModifier(); }
//~ End UInventory Interface.

//~ Begin IDamageLogInterface Interface
public:
	virtual FText GetDamageLogInstigatorName() const override { return GetWeaponName(); }
//~ End IDamageLogInterface Interface

public:
	UFUNCTION(BlueprintCallable, Category = Weapon)
	EWeaponState GetWeaponState() const { return WeaponState; }

	//This might be overkill but the purpose of this function is just to make sure there's an UWeapon::IsActive function in the "Weapon" category in Blueprint.
	UFUNCTION(BlueprintCallable, Category = Weapon, meta = (DisplayName="Is Active", ScriptName="IsActive"))
	bool K2_IsWeaponActive() const { return WeaponState == EWeaponState::Active; }
	UFUNCTION(BlueprintCallable, Category = Weapon)
	bool IsInactive() const { return WeaponState == EWeaponState::Inactive; }

	UFUNCTION(BlueprintCallable, Category = Weapon)
	bool IsCurrentlyEquippedWeapon() const;
	
	UFUNCTION(BlueprintPure, Category = Weapon)
	bool IsEquipping() const { return WeaponState == EWeaponState::Equipping; }
	UFUNCTION(BlueprintPure, Category = Weapon)
	bool IsPuttingDown() const { return WeaponState == EWeaponState::PuttingDown; }
	UFUNCTION(BlueprintPure, Category = Weapon)
	bool IsPendingPutDown() const { return bPendingPutDown; }
	UFUNCTION(BlueprintPure, Category = Weapon)
	virtual bool IsFiring() const;

	UFUNCTION(BlueprintPure, Category = Weapon)
	float GetEquipDuration() const;
	UFUNCTION(BlueprintPure, Category = Weapon)
	float GetPutDownDuration() const;

	UFUNCTION(BlueprintPure, Category = Weapon)
	const TArray<UFireMode*>& GetFireModeList() const { return FireModeList; }
	UFUNCTION(BlueprintPure, Category = Weapon)
	UFireMode* GetFireMode(EFireMode Mode) const;
	UFUNCTION(BlueprintPure, Category = Weapon)
	UAmmo* GetAmmo(EFireMode Mode) const;
	
	UFUNCTION()
	const UFireMode* GetFireModeDefaultObject(EFireMode Mode) const;
	UFUNCTION()
	const UAmmo* GetAmmoDefaultObject(EFireMode Mode) const;

	UFUNCTION(BlueprintPure, Category = Weapon)
	EFireMode GetFireModeEnum(UFireMode* FireMode) const;
	UFUNCTION(BlueprintPure, Category = Weapon)
	EFireMode GetAmmoFireModeEnum(UAmmo* Ammo) const;

	//Mesh and firing effects
	UFUNCTION(BlueprintPure, Category = Weapon)
	bool ShouldPlayEffects1P() const;
	UFUNCTION(BlueprintPure, Category = Weapon)
	bool ShouldPlayEffects3P() const;
	UFUNCTION(BlueprintPure, Category = Weapon)
	USkeletalMeshComponent* GetMesh1P() const;
	UFUNCTION(BlueprintPure, Category = Weapon)
	USkeletalMeshComponent* GetWeaponMesh1P() const;
	UFUNCTION(BlueprintPure, Category = Weapon)
	USkeletalMeshComponent* GetMesh3P() const;
	UFUNCTION(BlueprintPure, Category = Weapon)
	USkeletalMeshComponent* GetWeaponMesh3P() const;
	
	UFUNCTION(BlueprintPure, Category = Weapon)
	USkeletalMesh* GetSkeletalMesh1P() const { return FirstPersonMesh; }
	UFUNCTION(BlueprintPure, Category = Weapon)
	USkeletalMesh* GetSkeletalMesh3P() const { return ThirdPersonMesh; }
	UFUNCTION(BlueprintPure, Category = Weapon)
	TSubclassOf<UAnimInstance> GetWeaponAnimInstance1P() const { return WeaponAnimInstance1P; }
	UFUNCTION(BlueprintPure, Category = Weapon)
	TSubclassOf<UAnimInstance> GetWeaponAnimInstance3P() const { return WeaponAnimInstance3P; }

	//UI and general display info
	UFUNCTION(BlueprintCallable, Category = Weapon)
	FText GetWeaponName() const { return WeaponName; }
	UFUNCTION(BlueprintCallable, Category = Weapon)
	FText GetWeaponDescription() const { return WeaponDescription; }
	UFUNCTION(BlueprintCallable, Category = Weapon)
	TSoftObjectPtr<UTexture> GetInventoryItemImage() const { return InventoryItemImage; }
	UFUNCTION(BlueprintCallable, Category = Weapon)
	TSoftClassPtr<UWeaponUserWidget> GetInventoryItemWidget() const { return InventoryItemWidget; }
		
	UFUNCTION(BlueprintCallable, Category = Weapon)
	uint8 GetWeaponPriority() const { return WeaponPriority; }
	UFUNCTION(BlueprintCallable, Category = Weapon)
	EWeaponGroup GetWeaponGroup() const { return WeaponGroup; }
	UFUNCTION(BlueprintCallable, Category = Weapon)
	bool ShouldAvoidInitialEquip() const { return bShouldAvoidInitialEquip; }

	UFUNCTION()
	virtual bool Fire(EFireMode Mode);
	UFUNCTION()
	virtual bool StopFire(EFireMode Mode);
	UFUNCTION()
	virtual void StopAllFire();

	UFUNCTION()
	virtual bool Reload(EFireMode Mode);
	UFUNCTION()
	virtual bool StopReload(EFireMode Mode);
	UFUNCTION()
	virtual void StopAllReload();
	
	//Returns true if the requesting firemode cannot perform any actions (firing, reloading, weapon switching).
	UFUNCTION()
	virtual bool IsActionBlocked(const UFireMode* InstigatorFireMode = nullptr) const;

	//Special logic pretaining to firing specific or reloading specific blocking can be done here (still listens to IsActionBlocked());
	UFUNCTION()
	virtual bool CanFire(const UFireMode* InstigatorFireMode = nullptr) const;
	UFUNCTION()
	virtual bool CanReload(UFireMode* InstigatorFireMode = nullptr) const;

	UFUNCTION()
	virtual bool Equip();
	UFUNCTION(BlueprintCallable, Category = Weapon)
	virtual bool CanEquip() const;
	UFUNCTION()
	virtual void EquipComplete();
	UFUNCTION()
	virtual void OnEquipCosmetic();

	UFUNCTION()
	virtual bool PutDown();
	UFUNCTION(BlueprintCallable, Category = Weapon)
	virtual bool CanPutDown() const;
	UFUNCTION()
	virtual void ClearPendingPutDown();
	UFUNCTION()
	virtual void PutDownComplete();
	UFUNCTION()
	virtual void OnPutDownCosmetic();
	UFUNCTION()
	virtual void OnPutDownCompleteCosmetic();

	UFUNCTION()
	virtual float GetWeaponRating() const { return 1.f; }

	//----
	//Server/simulated proxy synchronization functions. Used to catch the server/simulated proxy up to the owning client. All of these should NEVER be called on the LOCALLY OWNED INSTANCE.
	UFUNCTION()
	virtual void AbortPutDown();
	UFUNCTION()
	virtual void AbortEquip();
	UFUNCTION()
	virtual void ForcePutDown();
	UFUNCTION()
	virtual void ForceEquip();

	//Callbacks used to start pending put downs.
	UFUNCTION()
	void FireCompleted(UFireMode* FireMode);
	UFUNCTION()
	void ReloadCompleted(UAmmo* Ammo);

	void RegisterFireMode(UFireMode* FireMode);

	UFUNCTION(BlueprintCallable, Category = Weapon)
	bool HasAnyDescriptors(UPARAM(meta=(Bitmask,BitmaskEnum=EWeaponDescriptor)) int32 Flags) const;
	UFUNCTION(BlueprintCallable, Category = Weapon)
	uint8 GetDescriptors() const { return WeaponDescriptor; }

public:
	UPROPERTY(BlueprintAssignable, Category = Weapon)
	FWeaponStateChangedSignature OnWeaponStateChanged;
	UPROPERTY(BlueprintAssignable, Category = Weapon)
	FWeaponEquipSignature OnWeaponEquipStart;
	UPROPERTY(BlueprintAssignable, Category = Weapon)
	FWeaponEquipSignature OnWeaponEquipComplete;
	UPROPERTY(BlueprintAssignable, Category = Weapon)
	FWeaponPutDownSignature OnWeaponPutDownStart;
	UPROPERTY(BlueprintAssignable, Category = Weapon)
	FWeaponPutDownSignature OnWeaponPutDownComplete;
	UPROPERTY(BlueprintAssignable, Category = Weapon)
	FWeaponFireCompleteSignature OnWeaponFireComplete;
	UPROPERTY(BlueprintAssignable, Category = Weapon)
	FWeaponReloadCompleteSignature OnWeaponReloadComplete;

	//Broadcasted when the weapon has completed some action and is now ready to perform a pending put down.
	UPROPERTY(BlueprintAssignable, Category = Weapon)
	FWeaponAwaitingPutDown OnWeaponAwaitingPutDown;

protected:
	UFUNCTION()
	virtual void SetWeaponState(EWeaponState State);

	UFUNCTION()
	virtual void OnRep_WeaponState(EWeaponState PreviousState);

	UFUNCTION()
	void InitializeFireModeList();

	UFUNCTION()
	void CheckPendingPutDown();

	UFUNCTION(BlueprintImplementableEvent, Category = Weapon, meta = (DisplayName = "On Equip", ScriptName = "OnEquip"))
	void K2_OnEquip();
	UFUNCTION(BlueprintImplementableEvent, Category = Weapon, meta = (DisplayName = "On Equip Complete", ScriptName = "OnEquipComplete"))
	void K2_OnEquipComplete();
	UFUNCTION(BlueprintImplementableEvent, Category = Weapon, meta = (DisplayName = "On Put Down", ScriptName = "OnPutDown"))
	void K2_OnPutDown();
	UFUNCTION(BlueprintImplementableEvent, Category = Weapon, meta = (DisplayName = "On Put Down Complete", ScriptName = "OnPutDownComplete"))
	void K2_OnPutDownComplete();

protected:
	UPROPERTY(EditDefaultsOnly, Category = Inventory)
	float EquippedMovementSpeedModifier = 1.f;

	//When not this weapon is not active, weapons can opt in to still replicate fire mode data. This is a performance optimization.
	UPROPERTY(EditDefaultsOnly, Category = Weapon)
	bool bReplicateFireModesWhenInactive = false;

	UPROPERTY(EditDefaultsOnly, Category = Weapon)
	float EquipTime = 1.f;	
	UPROPERTY(EditDefaultsOnly, Category = Weapon)
	float PutDownTime = 1.f;

	//Used by player classes to determine bonuses.
	UPROPERTY(EditDefaultsOnly, Category = Weapon, meta = (Bitmask, BitmaskEnum = EWeaponDescriptor))
	uint8 WeaponDescriptor = 0;

	UPROPERTY(EditDefaultsOnly, Category = Equip)
	uint8 WeaponPriority = 0;
	UPROPERTY(EditDefaultsOnly, Category = Equip)
	EWeaponGroup WeaponGroup = EWeaponGroup::None;
	UPROPERTY(EditDefaultsOnly, Category = Equip)
	bool bShouldAvoidInitialEquip = false;

	UPROPERTY(EditDefaultsOnly, Category = UI)
	FText WeaponName;
	UPROPERTY(EditDefaultsOnly, Category = UI)
	FText WeaponDescription;
	UPROPERTY(EditDefaultsOnly, Category = UI)
	TSoftObjectPtr<UTexture> InventoryItemImage;
	UPROPERTY(EditDefaultsOnly, Category = UI)
	TSoftClassPtr<UWeaponUserWidget> InventoryItemWidget;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = FireMode)
	UFireMode* PrimaryFire = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = FireMode)
	UFireMode* SecondaryFire = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = FireMode)
	UFireMode* TertiaryFire = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = FireMode)
	UFireMode* QuaternaryFire = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = FireMode)
	UFireMode* QuinaryFire = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = Mesh)
	USkeletalMesh* FirstPersonMesh = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Mesh)
	FTransform FirstPersonMeshAttachTransform = FTransform();
	UPROPERTY(EditDefaultsOnly, Category = Mesh)
	USkeletalMesh* ThirdPersonMesh = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Mesh)
	FTransform ThirdPersonMeshAttachTransform = FTransform();

	UPROPERTY(EditDefaultsOnly, Category = Mesh)
	TSubclassOf<UAnimInstance> WeaponAnimInstance1P = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Mesh)
	TSubclassOf<UAnimInstance> WeaponAnimInstance3P = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = Mesh, meta = (DisplayName = "First Person Animations"))
	TSubclassOf<UAnimationObject> FirstPersonAnimation = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Mesh, meta = (DisplayName = "Third Person Animations"))
	TSubclassOf<UAnimationObject> ThirdPersonAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bMakeNoiseOnEquip = false;
	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (EditCondition = "bMakeNoiseOnEquip", DisplayName = "Make Noise On Equip"))
	FCoreNoiseParams EquipNoise = FCoreNoiseParams(CoreNoiseTag::WeaponEquip, 0.1f, 0.f);

	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bMakeNoiseOnPutDown = false;
	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (EditCondition = "bMakeNoiseOnPutDown", DisplayName = "Make Noise On Put Down"))
	FCoreNoiseParams PutDownNoise = FCoreNoiseParams(CoreNoiseTag::WeaponPutDown, 0.1f, 0.f);

private:
	UPROPERTY(Transient, ReplicatedUsing = OnRep_WeaponState)
	EWeaponState WeaponState = EWeaponState::Inactive;

	UPROPERTY(Transient)
	TArray<UFireMode*> FireModeList;

	UPROPERTY(Transient)
	bool bPendingPutDown = false;

	UPROPERTY(Transient)
	FTimerHandle EquipTimer;
	UPROPERTY(Transient)
	FTimerHandle PutDownTimer;

public:
	UFUNCTION(BlueprintCallable, Category = Weapon)
	static TArray<TSubclassOf<UWeapon>> SortWeaponClassList(const TArray<TSubclassOf<UWeapon>>& WeaponClassList) { TArray<TSubclassOf<UWeapon>> SortedList = WeaponClassList; SortedList.Sort(UWeapon::FSortByPriority()); return SortedList; }
	UFUNCTION(BlueprintCallable, Category = Weapon)
	static TArray<UWeapon*> SortWeaponList(const TArray<UWeapon*>& WeaponList) { TArray<UWeapon*> SortedList = WeaponList; SortedList.Sort(UWeapon::FSortByPriority()); return SortedList; }

	UFUNCTION(BlueprintCallable, Category = Weapon)
	static FText GetWeaponNameFromClass(TSubclassOf<UWeapon> WeaponClass);
	UFUNCTION(BlueprintCallable, Category = Weapon)
	static FText GetWeaponDescriptionFromClass(TSubclassOf<UWeapon> WeaponClass);
	UFUNCTION(BlueprintCallable, Category = Weapon)
	static TSoftObjectPtr<UTexture> GetInventoryItemImageFromClass(TSubclassOf<UWeapon> WeaponClass);
	UFUNCTION(BlueprintCallable, Category = Weapon)
	static EWeaponGroup GetWeaponGroupFromClass(TSubclassOf<UWeapon> WeaponClass);

	UFUNCTION(BlueprintCallable, Category = Weapon)
	static FText GetWeaponStateName(EWeaponState State);
	
	UFUNCTION(BlueprintCallable, Category = Weapon)
	static FText GetFireModeName(EFireMode FireMode);

	struct FSortByPriority
	{
		FSortByPriority() {}

		bool operator()(const TWeakObjectPtr<UWeapon> A, const TWeakObjectPtr<UWeapon> B) const
		{
			return IsHigherPriority(A.Get(), B.Get());
		}

		bool operator()(const TSubclassOf<UWeapon> A, const TSubclassOf<UWeapon> B) const
		{
			return IsHigherPriority(A.GetDefaultObject(), B.GetDefaultObject());
		}

		bool operator()(const UWeapon& A, const UWeapon& B) const
		{
			return IsHigherPriority(&A, &B);
		}

	private:
		FORCEINLINE bool IsHigherPriority(const UWeapon* A, const UWeapon* B) const
		{
			if (!B)
			{
				return true;
			}

			if (!A)
			{
				return false;
			}

			if (A->GetWeaponPriority() != B->GetWeaponPriority())
			{
				return A->GetWeaponPriority() > B->GetWeaponPriority();
			}

			if (A->GetWeaponGroup() != B->GetWeaponGroup())
			{
				return A->GetWeaponGroup() > B->GetWeaponGroup();
			}

			return A->GetUniqueID() < B->GetUniqueID();
		}
	};
};