// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/CoreUserWidget.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "WeaponUserWidget.generated.h"

class UWeapon;

/**
 * 
 */
UCLASS(NotBlueprintable)
class NAUSEA_API UBaseWeaponUserWidget : public UCoreUserWidget
{
	GENERATED_UCLASS_BODY()

//~ Begin UUserWidget Interface
public:
	virtual bool Initialize() override;
	virtual void NativePreConstruct() override;
//~ End UUserWidget Interface

public:
	UFUNCTION(BlueprintCallable, Category = WeaponUserWidget)
	virtual UWeapon* GetOwningWeapon() const { PURE_VIRTUAL(UBaseWeaponUserWidget::GetOwningWeapon, return nullptr;) }

	UFUNCTION()
	void ReceiveWeaponPutDown(UWeapon* Weapon);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Weapon User Widget")
	void OnWeaponPutDown(UWeapon* Weapon);

protected:
	virtual void MovePointerToWeakObjectPointer() { PURE_VIRTUAL(UBaseWeaponUserWidget::MovePointerToWeakObjectPointer) }

private:
	UPROPERTY()
	bool bHasPerformedFirstPreConsturct = false;
};


UCLASS(Blueprintable)
class NAUSEA_API UWeaponUserWidget : public UBaseWeaponUserWidget
{
	GENERATED_UCLASS_BODY()

//~ Begin UBaseWeaponUserWidget Interface
public:
	virtual UWeapon* GetOwningWeapon() const;
protected:
	virtual void MovePointerToWeakObjectPointer() override;
//~ End UBaseWeaponUserWidget Interface

protected:
	UPROPERTY(BlueprintGetter=GetOwningWeapon, meta = (ExposeOnSpawn = true))
	UWeapon* Weapon = nullptr;
	UPROPERTY()
	TWeakObjectPtr<UWeapon> WeakWeapon = nullptr;
};

class UFireMode;

UCLASS(Blueprintable)
class NAUSEA_API UFireModeUserWidget : public UBaseWeaponUserWidget
{
	GENERATED_UCLASS_BODY()

//~ Begin UBaseWeaponUserWidget Interface
public:
	virtual UWeapon* GetOwningWeapon() const override;
protected:
	virtual void MovePointerToWeakObjectPointer() override;
//~ End UBaseWeaponUserWidget Interface

	UFUNCTION(BlueprintCallable, Category = FireModeUserWidget)
	UFireMode* GetOwningFireMode() const;

protected:
	UPROPERTY(BlueprintGetter=GetOwningFireMode, meta = (ExposeOnSpawn = true))
	UFireMode* FireMode = nullptr;
	UPROPERTY()
	TWeakObjectPtr<UFireMode> WeakFireMode = nullptr;
};

class UAmmo;

UCLASS(Blueprintable)
class NAUSEA_API UAmmoUserWidget : public UBaseWeaponUserWidget
{
	GENERATED_UCLASS_BODY()

//~ Begin UBaseWeaponUserWidget Interface
public:
	virtual UWeapon* GetOwningWeapon() const override;
protected:
	virtual void MovePointerToWeakObjectPointer() override;
//~ End UBaseWeaponUserWidget Interface
	
	UFUNCTION(BlueprintCallable, Category = AmmoUserWidget)
	UAmmo* GetOwningAmmo() const;

protected:
	UPROPERTY(BlueprintGetter=GetOwningAmmo, meta = (ExposeOnSpawn = true))
	UAmmo* Ammo = nullptr;
	UPROPERTY()
	TWeakObjectPtr<UAmmo> WeakAmmo = nullptr;
};


DECLARE_DELEGATE(FLoadWidgetDelegate);

UCLASS()
class NAUSEA_API UBaseWeaponWidgetAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UBlueprintAsyncActionBase Interface
	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;
//~ End UBlueprintAsyncActionBase Interface

protected:
	UFUNCTION()
	void OnWeaponPutDown(UWeapon* Weapon);

	template<class TAsyncActionClass>
	static TAsyncActionClass* CreateWeaponWidgetAsyncAction(UWeapon* Weapon, TSoftClassPtr<UBaseWeaponUserWidget> WidgetClass, bool bInEquippedOnly);

protected:
	UPROPERTY()
	TWeakObjectPtr<UWeapon> OwningWeapon;

	UPROPERTY()
	TSoftClassPtr<UBaseWeaponUserWidget> SoftWidgetClass;

	TSharedPtr<FStreamableHandle> StreamableHandle;

	FLoadWidgetDelegate LoadDelegate;

	UPROPERTY()
	bool bFailed = false;
	
	//If true, this async action will cancel/not fire off the completion delegate if the weapon is not equipped.
	UPROPERTY()
	bool bEquippedOnly = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponWidgetLoadedSignature, TSubclassOf<UWeaponUserWidget>, WidgetClass);

UCLASS()
class NAUSEA_API UWeaponWidgetAsyncAction : public UBaseWeaponWidgetAsyncAction
{
	GENERATED_UCLASS_BODY()
	
public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = UI)
	static UWeaponWidgetAsyncAction* LoadWeaponWidget(UWeapon* Weapon, TSoftClassPtr<UWeaponUserWidget> WidgetClass, bool bMustBeEquipped = true);

public:
	UPROPERTY(BlueprintAssignable)
	FOnWeaponWidgetLoadedSignature OnWeaponWidgetLoaded;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAmmoWidgetLoadedSignature, TSubclassOf<UAmmoUserWidget>, WidgetClass);

UCLASS()
class NAUSEA_API UWeaponAmmoWidgetAsyncAction : public UBaseWeaponWidgetAsyncAction
{
	GENERATED_UCLASS_BODY()
	
public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = UI)
	static UWeaponAmmoWidgetAsyncAction* LoadAmmoWidget(UAmmo* Ammo, TSoftClassPtr<UAmmoUserWidget> WidgetClass, bool bMustBeEquipped = true);

public:
	UPROPERTY(BlueprintAssignable)
	FOnAmmoWidgetLoadedSignature OnAmmoWidgetLoaded;
};