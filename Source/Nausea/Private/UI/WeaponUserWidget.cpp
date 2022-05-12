// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "UI/WeaponUserWidget.h"
#include "Weapon/Weapon.h"
#include "Weapon/FireMode.h"
#include "Weapon/FireMode/Ammo.h"

UBaseWeaponUserWidget::UBaseWeaponUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

bool UBaseWeaponUserWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	return true;
}

void UBaseWeaponUserWidget::NativePreConstruct()
{
	if (!bHasPerformedFirstPreConsturct)
	{
		MovePointerToWeakObjectPointer();

		if (GetOwningWeapon())
		{
			GetOwningWeapon()->OnWeaponPutDownStart.AddDynamic(this, &UBaseWeaponUserWidget::ReceiveWeaponPutDown);
		}
		bHasPerformedFirstPreConsturct = true;
	}

	Super::NativePreConstruct();
}

void UBaseWeaponUserWidget::ReceiveWeaponPutDown(UWeapon* Weapon)
{
	check(Weapon == GetOwningWeapon());

	OnWeaponPutDown(GetOwningWeapon());
}

UWeaponUserWidget::UWeaponUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UWeapon* UWeaponUserWidget::GetOwningWeapon() const
{
	return WeakWeapon.IsValid() ? WeakWeapon.Get() : Weapon;
}

void UWeaponUserWidget::MovePointerToWeakObjectPointer()
{
	WeakWeapon = Weapon;
	Weapon = nullptr;
}

UFireModeUserWidget::UFireModeUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UWeapon* UFireModeUserWidget::GetOwningWeapon() const
{
	if (WeakFireMode.IsValid())
	{
		return WeakFireMode->GetOwningWeapon();
	}

	if (FireMode)
	{
		return FireMode->GetOwningWeapon();
	}

	return nullptr;
}

void UFireModeUserWidget::MovePointerToWeakObjectPointer()
{
	WeakFireMode = FireMode;
	FireMode = nullptr;
}

UFireMode* UFireModeUserWidget::GetOwningFireMode() const
{
	return WeakFireMode.IsValid() ? WeakFireMode.Get() : FireMode;
}

UAmmoUserWidget::UAmmoUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UWeapon* UAmmoUserWidget::GetOwningWeapon() const
{
	if (WeakAmmo.IsValid())
	{
		return WeakAmmo->GetOwningWeapon();
	}

	if (Ammo)
	{
		return Ammo->GetOwningWeapon();
	}

	return nullptr;
}

void UAmmoUserWidget::MovePointerToWeakObjectPointer()
{
	WeakAmmo = Ammo;
	Ammo = nullptr;
}

UAmmo* UAmmoUserWidget::GetOwningAmmo() const
{
	return WeakAmmo.IsValid() ? WeakAmmo.Get() : Ammo;
}

UBaseWeaponWidgetAsyncAction::UBaseWeaponWidgetAsyncAction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

template<class TAsyncActionClass>
TAsyncActionClass* UBaseWeaponWidgetAsyncAction::CreateWeaponWidgetAsyncAction(UWeapon* Weapon, TSoftClassPtr<UBaseWeaponUserWidget> WidgetClass, bool bInEquippedOnly)
{
	TAsyncActionClass* Action = NewObject<TAsyncActionClass>();
	Action->OwningWeapon = Weapon;
	Action->SoftWidgetClass = WidgetClass;
	Action->bEquippedOnly = bInEquippedOnly;
	return Action;
}

void UBaseWeaponWidgetAsyncAction::Activate()
{
	if (!OwningWeapon.IsValid() || SoftWidgetClass.IsNull() || !LoadDelegate.IsBound())
	{
		bFailed = true;
		SetReadyToDestroy();
		return;
	}

	//If already loaded, don't bother with the streamable manager.
	if (SoftWidgetClass.Get())
	{
		TWeakObjectPtr<UBaseWeaponWidgetAsyncAction> WeakThis = this;
		OwningWeapon->GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [WeakThis]()
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			if (WeakThis->LoadDelegate.IsBound())
			{
				WeakThis->LoadDelegate.ExecuteIfBound();
			}
			
			WeakThis->SetReadyToDestroy();
		}));
		return;
	}

	UAssetManager& AssetManager = UAssetManager::Get();

	if (!AssetManager.IsValid())
	{
		bFailed = true;
		SetReadyToDestroy();
		return;
	}
	
	FStreamableManager& StreamableManager = AssetManager.GetStreamableManager();
	StreamableHandle = StreamableManager.RequestAsyncLoad(SoftWidgetClass.ToSoftObjectPath());

	if (!StreamableHandle.IsValid())
	{
		bFailed = true;
		SetReadyToDestroy();
		return;
	}

	TWeakObjectPtr<UBaseWeaponWidgetAsyncAction> WeakThis = TWeakObjectPtr<UBaseWeaponWidgetAsyncAction>(this);
	if (bEquippedOnly && !OwningWeapon->OnWeaponPutDownStart.IsAlreadyBound(this, &UBaseWeaponWidgetAsyncAction::OnWeaponPutDown))
	{
		OwningWeapon->OnWeaponPutDownStart.AddDynamic(this, &UBaseWeaponWidgetAsyncAction::OnWeaponPutDown);
	}

	auto StreamClassCompleteDelegate = [WeakThis] {
		if (!WeakThis.IsValid())
		{
			return;
		}

		if (!WeakThis->SoftWidgetClass.Get())
		{
			WeakThis->bFailed = true;
			WeakThis->SetReadyToDestroy();
		}

		if (WeakThis->bEquippedOnly && (!WeakThis->OwningWeapon.IsValid() || !WeakThis->OwningWeapon->IsCurrentlyEquippedWeapon() || WeakThis->OwningWeapon->IsPuttingDown()))
		{
			WeakThis->bFailed = true;
			WeakThis->SetReadyToDestroy();
			return;
		}

		if (WeakThis->LoadDelegate.IsBound())
		{
			WeakThis->LoadDelegate.ExecuteIfBound();
		}

		WeakThis->SetReadyToDestroy();
	};

	if (StreamableHandle->HasLoadCompleted())
	{
		StreamClassCompleteDelegate();
		return;
	}

	StreamableHandle->BindCompleteDelegate(FStreamableDelegate::CreateWeakLambda(this, StreamClassCompleteDelegate));
}

void UBaseWeaponWidgetAsyncAction::SetReadyToDestroy()
{
	if (StreamableHandle.IsValid())
	{
		if (StreamableHandle->IsLoadingInProgress())
		{
			StreamableHandle->CancelHandle();
		}

		StreamableHandle.Reset();
	}

	if (bEquippedOnly && OwningWeapon->OnWeaponPutDownStart.IsAlreadyBound(this, &UBaseWeaponWidgetAsyncAction::OnWeaponPutDown))
	{
		OwningWeapon->OnWeaponPutDownStart.RemoveDynamic(this, &UBaseWeaponWidgetAsyncAction::OnWeaponPutDown);
	}

	if (LoadDelegate.IsBound())
	{
		LoadDelegate.Unbind();
	}

	Super::SetReadyToDestroy();
}

void UBaseWeaponWidgetAsyncAction::OnWeaponPutDown(UWeapon* Weapon)
{
	if (!OwningWeapon.IsValid())
	{
		return;
	}

	bFailed = true;
	SetReadyToDestroy();
}

UWeaponWidgetAsyncAction::UWeaponWidgetAsyncAction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UWeaponWidgetAsyncAction* UWeaponWidgetAsyncAction::LoadWeaponWidget(UWeapon* Weapon, TSoftClassPtr<UWeaponUserWidget> WidgetClass, bool bMustBeEquipped)
{
	if (!Weapon)
	{
		return nullptr;
	}

	UWeaponWidgetAsyncAction* Action = CreateWeaponWidgetAsyncAction<UWeaponWidgetAsyncAction>(Weapon, WidgetClass, bMustBeEquipped);
	auto LoadWeaponWidgetDelegate = [Action] {
		Action->OnWeaponWidgetLoaded.Broadcast(Action->SoftWidgetClass.Get());
	};
	
	Action->LoadDelegate = FLoadWidgetDelegate::CreateWeakLambda(Action, LoadWeaponWidgetDelegate);
	Action->RegisterWithGameInstance(Weapon->GetWorld()->GetGameInstance());
	return Action;
}

UWeaponAmmoWidgetAsyncAction::UWeaponAmmoWidgetAsyncAction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UWeaponAmmoWidgetAsyncAction* UWeaponAmmoWidgetAsyncAction::LoadAmmoWidget(UAmmo* Ammo, TSoftClassPtr<UAmmoUserWidget> WidgetClass, bool bMustBeEquipped)
{
	if (!Ammo || !Ammo->GetOwningWeapon())
	{
		return nullptr;
	}

	UWeaponAmmoWidgetAsyncAction* Action = CreateWeaponWidgetAsyncAction<UWeaponAmmoWidgetAsyncAction>(Ammo->GetOwningWeapon(), WidgetClass, bMustBeEquipped);
	auto LoadAmmoWidgetDelegate = [Action] {
		Action->OnAmmoWidgetLoaded.Broadcast(Action->SoftWidgetClass.Get());
	};

	Action->LoadDelegate = FLoadWidgetDelegate::CreateWeakLambda(Action, LoadAmmoWidgetDelegate);
	Action->RegisterWithGameInstance(Ammo->GetOwningWeapon()->GetWorld()->GetGameInstance());
	return Action;
}