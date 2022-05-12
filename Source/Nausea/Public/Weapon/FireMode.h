// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Tickable.h"
#include "Weapon/WeaponTypes.h"
#include "Player/PlayerOwnershipInterface.h"
#include "System/ReplicatedObjectInterface.h"
#include "FireMode.generated.h"

class UWeapon;
class ACoreCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFireBeginSignature, UFireMode*, FireMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFireEndSignature, UFireMode*, FireMode);

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, AutoExpandCategories = (Default))
class NAUSEA_API UFireMode : public UObject, public FTickableGameObject, public IPlayerOwnershipInterface, public IReplicatedObjectInterface
{
	GENERATED_UCLASS_BODY()

//~ Begin UObject Interface
public:
	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;
	virtual UWorld* GetWorld() const override final { return (WorldPrivate ? WorldPrivate : GetWorld_Uncached()); } //UActorComponent's implementation
//~ End UObject Interface

//~ Begin FTickableGameObject Interface
protected:
	virtual void Tick(float DeltaTime) override { if (bK2TickImplemented) { K2_Tick(DeltaTime); } }
public:
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickable() const override { return bTickEnabled && !IsPendingKill(); }
	virtual TStatId GetStatId() const override { return TStatId(); }
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
//~ End FTickableGameObject Interface

//~ Begin IPlayerOwnershipInterface Interface.
public:
	virtual ACorePlayerState* GetOwningPlayerState() const override;
	virtual AController* GetOwningController() const override;
	virtual APawn* GetOwningPawn() const override;
//~ End IPlayerOwnershipInterface Interface.

public:
	virtual void RegisterOwningWeaponClass(const UWeapon* Weapon);
	TSubclassOf<UWeapon> GetOwningWeaponClass() const { return OwningWeaponClass; }

	bool IsInitialized() const { return OwningCharacter != nullptr; }
	void InitializeFireModeSlot(EFireMode Slot);
	virtual void Initialize(UWeapon* Weapon);

	UFUNCTION(BlueprintCallable, Category = FireMode)
	FORCEINLINE UWeapon* GetOwningWeapon() const { return OwningWeapon; }

	UFUNCTION(BlueprintCallable, Category = FireMode)
	FORCEINLINE EFireMode GetFireMode() const { return FireMode; }

	UFUNCTION(BlueprintCallable, Category = FireMode)
	FORCEINLINE ACoreCharacter* GetOwningCharacter() const { return OwningCharacter; }

	const UFireMode* GetDefaultObject() const;

public:
	//Used to know if we're a replicated firemode without casting to the replicated firemode every time.
	UFUNCTION(BlueprintCallable, Category = FireMode)
	virtual bool IsReplicated() const { return false; }

	UFUNCTION(BlueprintCallable, Category = FireMode)
	FORCEINLINE bool IsHoldingFire() const { return bHoldingFire; }

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = FireMode)
	bool IsFiring() const;
	//If returns false if put away should be blocked from occurring. If this can ever be false firemode should callback to weapon when its task is complete.
	UFUNCTION(BlueprintCallable, Category = FireMode)
	virtual bool CanPutDown() const;
	//Returns true if any weapon actions are being blocked by this firemode.
	UFUNCTION(BlueprintCallable, Category = FireMode)
	virtual bool BlockAction(const UFireMode* InstigatorFireMode) const;
	//Should we bypass checks on this fire mode to block other fire modes?
	UFUNCTION(BlueprintCallable, Category = FireMode)
	bool ShouldBlockOtherFireModeAction() const { return bBlockActionsWhenFiring; }
	//Should we bypass checks on other fire modes that might be blocking our fire?
	UFUNCTION(BlueprintCallable, Category = FireMode)
	bool IgnoresFireModeActionBlock() const { return bIgnoreFireModeActionBlock; }

	UFUNCTION(BlueprintCallable, Category = FireMode)
	bool HasAnyDescriptors(UPARAM(meta=(Bitmask,BitmaskEnum=EFireModeDescriptor)) int32 Flags) const;
	UFUNCTION(BlueprintCallable, Category = FireMode)
	uint8 GetDescriptors() const { return FireModeDescriptor; }

	UFUNCTION(BlueprintCallable, Category = FireMode)
	virtual bool CanFire() const;

	UFUNCTION()
	virtual bool Fire(float WorldTimeOverride = -1.f);
	UFUNCTION()
	virtual void StopFire(float WorldTimeOverride = -1.f);

	UFUNCTION()
	virtual void ForceEndFire();
	
	//These don't do anything for now. There's an implementation in UWeaponFire for reference.
	UFUNCTION()
	virtual bool Reload() { return false; }
	UFUNCTION()
	virtual bool StopReload() { return false; }

	UFUNCTION()
	void ClearHoldingFire() { bHoldingFire = false; }

	UFUNCTION(BlueprintCallable, Category = FireMode)
	TSoftClassPtr<UUserWidget> GetCrosshairWidget() const { return CrosshairWidget; }

public:
	UPROPERTY(BlueprintAssignable, Category = FireMode)
	FFireBeginSignature OnFireStart;
	UPROPERTY(BlueprintAssignable, Category = FireMode)
	FFireEndSignature OnFireComplete;

protected:
	//Called after a fire has been validated.
	UFUNCTION()
	virtual void PerformFire() {}
	UFUNCTION()
	virtual void OnFireCosmetic() {}
	UFUNCTION()
	virtual void FireComplete();

	UFUNCTION()
	virtual void BindWeaponEvents();
	UFUNCTION()
	virtual void UnBindWeaponEvents();

	UFUNCTION()
	virtual void WeaponEquipComplete(UWeapon* Weapon);

	UFUNCTION()
	virtual void WeaponPutDownStart(UWeapon* Weapon);

	UFUNCTION(BlueprintImplementableEvent, Category = FireMode, meta = (DisplayName="Can Fire",ScriptName="CanFire"))
	void K2_CanFire(bool& bCanFire) const;
	UFUNCTION(BlueprintImplementableEvent, Category = FireMode, meta = (DisplayName="On Fire",ScriptName="OnFire"))
	void K2_OnFire();
	UFUNCTION(BlueprintImplementableEvent, Category = FireMode, meta = (DisplayName="On Stop Fire",ScriptName="OnStopFire"))
	void K2_OnStopFire();	
	UFUNCTION(BlueprintImplementableEvent, Category = FireMode, meta = (DisplayName="On Fire Complete",ScriptName="OnFireComplete"))
	void K2_OnFireComplete();
	UFUNCTION(BlueprintImplementableEvent, Category = FireMode, meta = (DisplayName="Tick",ScriptName="Tick"))
	void K2_Tick(float DeltaTime);
	
	UFUNCTION(BlueprintCallable, Category = FireMode)
	void SetTickEnabled(bool bInTickEnabled) { bTickEnabled = bInTickEnabled; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = FireMode)
	bool bCanEverTick = false;
	UPROPERTY(EditDefaultsOnly, Category = FireMode)
	bool bNeverTickOnDedicatedServer = false;
	UPROPERTY(EditDefaultsOnly, Category = FireMode)
	bool bStartWithTickEnabled = false;
	UPROPERTY(Transient)
	bool bTickEnabled = false;
	UPROPERTY(Transient)
	bool bK2TickImplemented = false;

	ETickableTickType TickType = ETickableTickType::Never;
	
	//Should this fire mode stop other weapon actions when firing?
	UPROPERTY(EditDefaultsOnly, Category = FireMode)
	bool bBlockActionsWhenFiring = true;
	//Should this fire mode ignore other fire modes that are blocking fire?
	UPROPERTY(EditDefaultsOnly, Category = FireMode)
	bool bIgnoreFireModeActionBlock = false;

	//Used by player classes to determine bonuses.
	UPROPERTY(EditDefaultsOnly, Category = FireMode, meta = (Bitmask, BitmaskEnum = EFireModeDescriptor))
	uint8 FireModeDescriptor = 0;

	UPROPERTY(EditDefaultsOnly, Category = UI)
	TSoftClassPtr<UUserWidget> CrosshairWidget;

private:
	UWorld* GetWorld_Uncached() const;

private:
	UWorld* WorldPrivate = nullptr;

	UPROPERTY(Transient)
	UWeapon* OwningWeapon = nullptr;
	UPROPERTY(Transient)
	ACoreCharacter* OwningCharacter = nullptr;


	UPROPERTY()
	EFireMode FireMode = EFireMode::MAX;
	UPROPERTY(Transient)
	bool bHoldingFire = false;

	UPROPERTY()
	TSubclassOf<UWeapon> OwningWeaponClass = nullptr;
};
