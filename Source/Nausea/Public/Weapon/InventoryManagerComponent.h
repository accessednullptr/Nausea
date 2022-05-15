// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Character/CoreCharacterComponent.h"
#include "WeaponTypes.h"
#include "InventoryManagerComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogInventoryManager, Warning, All);

class UInputComponent;
class ACorePlayerState;
class UPlayerClassComponent;
class ACoreCharacter;
class UInventory;
class UWeapon;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryAddedSignature, UInventory*, Inventory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryRemovedSignature, UInventory*, Inventory);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryUpdateSignature, UInventoryManagerComponent*, InventoryManager);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryGroupUpdateSignature, EWeaponGroup, Group);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCurrentWeaponUpdateSignature, UWeapon*, CurrentWeapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPendingWeaponUpdateSignature, UWeapon*, PendingWeapon);

USTRUCT(BlueprintType)
struct NAUSEA_API FWeaponGroupArray
{
	GENERATED_USTRUCT_BODY()

public:
	typedef TArray<TWeakObjectPtr<UWeapon>> WeaponArrayType;
	
	FWeaponGroupArray() {}

	FWeaponGroupArray(const FWeaponGroupArray& InWeaponGroup)
	{
		WeaponArray = InWeaponGroup.WeaponArray;
	}

	FWeaponGroupArray(const WeaponArrayType& InArray)
	{
		WeaponArray = InArray;
	}

public:
	UPROPERTY()
	TArray<TWeakObjectPtr<UWeapon>> WeaponArray;

public:
	TWeakObjectPtr<UWeapon> &operator[] (int Index) { return WeaponArray[Index]; }

#if TARRAY_RANGED_FOR_CHECKS
	FORCEINLINE WeaponArrayType::RangedForIteratorType      begin() { return WeaponArrayType::RangedForIteratorType(WeaponArray.Num(), WeaponArray.GetData()); }
	FORCEINLINE WeaponArrayType::RangedForConstIteratorType begin() const { return WeaponArrayType::RangedForConstIteratorType(WeaponArray.Num(), WeaponArray.GetData()); }
	FORCEINLINE WeaponArrayType::RangedForIteratorType      end() { return WeaponArrayType::RangedForIteratorType(WeaponArray.Num(), WeaponArray.GetData() + WeaponArray.Num()); }
	FORCEINLINE WeaponArrayType::RangedForConstIteratorType end() const { return WeaponArrayType::RangedForConstIteratorType(WeaponArray.Num(), WeaponArray.GetData() + WeaponArray.Num()); }
#else
	FORCEINLINE WeaponArrayType::RangedForIteratorType      begin() { return WeaponArray.GetData(); }
	FORCEINLINE WeaponArrayType::RangedForConstIteratorType begin() const { return WeaponArray.GetData(); }
	FORCEINLINE WeaponArrayType::RangedForIteratorType      end() { return WeaponArray.GetData() + WeaponArray.Num(); }
	FORCEINLINE WeaponArrayType::RangedForConstIteratorType end() const { return WeaponArray.GetData() + WeaponArray.Num(); }
#endif
};

/**
 * Component responsible for holding/managing inventory as well as weapons.
 */
UCLASS(HideCategories = (ComponentTick, Collision, Tags, Variable, Activation, ComponentReplication, Cooking, Sockets, UserAssetData))
class NAUSEA_API UInventoryManagerComponent : public UCoreCharacterComponent
{
	GENERATED_UCLASS_BODY()

//~ Begin UActorComponent Interface
public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
//~ End UActorComponent Interface

public:
	UFUNCTION()
	void SetPlayerDefaults();
	UFUNCTION()
	void InitializeInventory();
	UFUNCTION()
	void BindToPlayerClass(ACorePlayerState* PlayerState);
	UFUNCTION()
	void OnPlayerClassChanged(ACorePlayerState* PlayerState, UPlayerClassComponent* PlayerClassComponent);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = InventoryManager)
	bool AddInventory(TSubclassOf<UInventory> InventoryClass, bool bDispatchOnRep = true);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = InventoryManager)
	bool RemoveInventory(TSubclassOf<UInventory> InventoryClass, bool bDispatchOnRep = true);

	UFUNCTION()
	void SetupInputComponent(UInputComponent* InputComponent);

	UFUNCTION()
	void EquipNextWeapon(EWeaponGroup Group);
	
	UFUNCTION()
	void SetCurrentWeapon(UWeapon* DesiredWeapon);
	UFUNCTION()
	void SetPendingWeapon(UWeapon* DesiredWeapon);

	UFUNCTION()
	void StartFire(EFireMode FireMode);
	UFUNCTION()
	void StopFire(EFireMode FireMode);
	UFUNCTION()
	void StartReload(EFireMode FireMode);
	UFUNCTION()
	void StopReload(EFireMode FireMode);

	UFUNCTION(BlueprintCallable, Category = InventoryManager)
	FORCEINLINE UWeapon* GetCurrentWeapon() const { return CurrentWeapon; }
	UFUNCTION(BlueprintCallable, Category = InventoryManager)
	FORCEINLINE UWeapon* GetPendingWeapon() const { return PendingWeapon; }
	//Gets best weapon that is not currently equipped.

	UFUNCTION(BlueprintCallable, Category = InventoryManager)
	float GetMovementSpeedModifier() const;
	UFUNCTION(BlueprintCallable, Category = InventoryManager)
	void RequestMovementSpeedModifierUpdate() { bUpdateMovementSpeedModifier = true; }

	UFUNCTION()
	virtual UWeapon* GetNextBestWeapon() const;

	UFUNCTION(BlueprintCallable, Category = InventoryManager)
	bool ContainsInventory(UInventory* Inventory) const;
	UFUNCTION(BlueprintCallable, Category = InventoryManager)
	bool CanEquipWeapon(UWeapon* Weapon) const;
	UFUNCTION(BlueprintCallable, Category = InventoryManager)
	bool IsActionBlocked() const;

	UFUNCTION(BlueprintCallable, Category = InventoryManager)
	const TMap<EWeaponGroup, FWeaponGroupArray>& GetWeaponGroupMap() const { return WeaponGroupMap; }

	//Returns true if a UInventoryManagerComponent::EquipNextWeapon should be blocked (typically by UVoiceCommandComponent menu input).
	UFUNCTION(BlueprintCallable, Category = InventoryManager)
	bool IsEquipNextWeaponCategoryBlocked(EWeaponGroup WeaponGroup) const;

	UFUNCTION(BlueprintCallable, Category = Inventory, meta = (DisplayName = "Get Array", ScriptName = "GetArray"))
	static void GetArrayFromWeaponGroup(const FWeaponGroupArray& WeaponGroup, TArray<UWeapon*>& Array);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Inventory)
	static FText GetWeaponGroupTitle(EWeaponGroup WeaponGroup);

public:
	UPROPERTY(BlueprintAssignable, Category = Inventory)
	FInventoryAddedSignature OnInventoryAdded;
	UPROPERTY(BlueprintAssignable, Category = Inventory)
	FInventoryRemovedSignature OnInventoryRemoved;

	UPROPERTY(BlueprintAssignable, Category = Inventory)
	FInventoryUpdateSignature OnInventoryUpdate;
	UPROPERTY(BlueprintAssignable, Category = Inventory)
	FInventoryGroupUpdateSignature OnInventoryGroupUpdate;

	UPROPERTY(BlueprintAssignable, Category = Inventory)
	FCurrentWeaponUpdateSignature OnCurrentWeaponUpdate;
	UPROPERTY(BlueprintAssignable, Category = Inventory)
	FPendingWeaponUpdateSignature OnPendingWeaponUpdate;

protected:
	template<EFireMode Index>
	void StartFire()
	{
		StartFire(Index);
	}
	template<EFireMode Index>
	void StopFire()
	{
		StopFire(Index);
	}

	template<EFireMode Index>
	void StartReload()
	{
		StartReload(Index);
	}
	template<EFireMode Index>
	void StopReload()
	{
		StopReload(Index);
	}

	template<EWeaponGroup Group>
	void EquipNextWeapon()
	{
		EquipNextWeapon(Group);
	}

	UFUNCTION()
	void WeaponEquipComplete(UWeapon* Weapon);
	UFUNCTION()
	void WeaponPutDownComplete(UWeapon* Weapon);
	UFUNCTION()
	void ChangedWeapon();
	UFUNCTION()
	void OnWeaponChangedCosmetic(UWeapon* Weapon);

	void BindWeaponEvents();
	void UnBindWeaponEvents();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Reliable_SetCurrentWeapon(UWeapon* DesiredWeapon);
	//Used to synchronize the server to the client.
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Reliable_WeaponEquipped(UWeapon* DesiredWeapon);
	UFUNCTION()
	void WeaponAwaitingPutDown(UWeapon* Weapon);

	UFUNCTION(Client, Reliable)
	void Client_Reliable_CurrentWeaponSet(UWeapon* Weapon);
	UFUNCTION()
	void CheckWeaponSynchronization();

	UFUNCTION()
	void OnRep_InventoryList();
	UFUNCTION()
	void EquipInitialWeapon();
	UFUNCTION()
	void UpdateWeaponGroupMap();

	UFUNCTION()
	void OnInventoryEndPlay(UCoreCharacterComponent* Component, EEndPlayReason::Type Reason);

	UFUNCTION()
	void OnCharacterDied(UStatusComponent* Component, float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
	UFUNCTION()
	void OnActionInterrupt(const UStatusComponent* Component);

protected:
	//Currently equipped weapon
	UPROPERTY(Transient)
	UWeapon* CurrentWeapon = NULL;
	UPROPERTY(Transient)
	UWeapon* PendingWeapon = NULL;
	UPROPERTY(Transient)
	bool bHasEquipedInitialWeapon = false;

	UPROPERTY(EditDefaultsOnly, Category = Inventory)
	TArray<TSubclassOf<UInventory>> DefaultInventoryList;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_InventoryList)
	TArray<UInventory*> InventoryList;
	UPROPERTY(Transient)
	TArray<UInventory*> PreviousInventoryList;

	UPROPERTY(Transient)
	TMap<EWeaponGroup, FWeaponGroupArray> WeaponGroupMap;

	UPROPERTY(Transient)
	mutable float CachedMovementSpeedModifier = 1.f;
	UPROPERTY(Transient)
	mutable bool bUpdateMovementSpeedModifier = true;

private:
	UPROPERTY(Transient)
	UInputComponent* CurrentInputComponent = nullptr;
	UPROPERTY(Transient)
	TSet<EFireMode> CurrentlyHeldFireSet;
	UPROPERTY(Transient)
	TWeakObjectPtr<UStatusComponent> OwningStatusComponent = nullptr;

	UPROPERTY(Transient)
	UWeapon* CurrentServerWeapon = NULL;
};
