// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Weapon/InventoryManagerComponent.h"
#include "Internationalization/StringTableRegistry.h"
#include "GameFramework/GameStateBase.h"
#include "System/NetHelper.h"
#include "System/CoreGameplayStatics.h"
#include "Player/NauseaPlayerState.h"
#include "Player/PlayerClassComponent.h"
#include "Components/InputComponent.h"
#include "Character/CoreCharacter.h"
#include "Character/CoreCharacterMovementComponent.h"
#include "Gameplay/StatusComponent.h"
#include "Weapon/Inventory.h"
#include "Weapon/Weapon.h"
#include "Character/VoiceComponent.h"

DEFINE_LOG_CATEGORY(LogInventoryManager);

//Helper log. Should be used after an event log to describe general state information about a given weapon.
#define LOG_WEAPON_STATE(WeaponVariable, Verbosity)\
UE_LOG(LogInventoryManager, Verbosity, TEXT("    - " #WeaponVariable ": %s | State: %s | Pending Putdown: %s"), *GetNameSafe(WeaponVariable), *(WeaponVariable ? UWeapon::GetWeaponStateName(WeaponVariable->GetWeaponState()).ToString() : FString("None")), *(WeaponVariable && WeaponVariable->IsPendingPutDown() ? FString("true") : FString("false")));\

UInventoryManagerComponent::UInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);

	WeaponGroupMap.Reserve(MAXWEAPONGROUP);
}

void UInventoryManagerComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(UInventoryManagerComponent, InventoryList, PushReplicationParams::Default);
}

void UInventoryManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetNetMode() != NM_DedicatedServer)
	{
		OnCurrentWeaponUpdate.AddDynamic(this, &UInventoryManagerComponent::OnWeaponChangedCosmetic);
	}

	//Authority will do this via APawn::SetPlayerDefaults so that our context (namely controller/playerstate) will be available.
	if (GetOwnerRole() != ROLE_Authority)
	{
		InitializeInventory();
	}
}

void UInventoryManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		//Cleanup inventory array.
		if (InventoryList.Num() > 0)
		{
			for (int32 Index = InventoryList.Num() + 1; Index >= 0; Index--)
			{
				if (!InventoryList.IsValidIndex(Index) || !InventoryList[Index])
				{
					continue;
				}

				InventoryList[Index]->DestroyComponent();
			}

			OnRep_InventoryList(); //Call one OnRep here to "flush" the previous inventory out.
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UInventoryManagerComponent::SetPlayerDefaults()
{
	InitializeInventory();
}

void UInventoryManagerComponent::InitializeInventory()
{
	UpdateOwningCharacter();

	if (ACorePlayerState* CorePlayerState = Cast<ACorePlayerState>(GetOwningCharacter()->GetOwningPlayerState()))
	{
		BindToPlayerClass(CorePlayerState);
	}

	if (UStatusComponent* StatusComponent = GetOwningCharacter() ? GetOwningCharacter()->GetStatusComponent() : nullptr)
	{
		ensureMsgf(!StatusComponent->IsPendingKill(), TEXT("Status Component is pending kill."));
		ensureMsgf(!IsPendingKill(), TEXT("Inventory Component is pending kill."));

		OwningStatusComponent = GetOwningCharacter()->GetStatusComponent();
		
		StatusComponent->OnDied.AddDynamic(this, &UInventoryManagerComponent::OnCharacterDied);
		StatusComponent->OnActionInterrupt.AddUObject(this, &UInventoryManagerComponent::OnActionInterrupt);
	}

	if (GetOwnerRole() == ROLE_Authority)
	{
		TArray<TSubclassOf<UInventory>> InventoryClassList = DefaultInventoryList;
		if (UPlayerClassComponent* PlayerClassComponent = GetOwningCharacter()->GetPlayerClassComponent())
		{
			PlayerClassComponent->ProcessInitialInventoryList(InventoryClassList);
		}

		for (TSubclassOf<UInventory> Inventory : InventoryClassList)
		{
			AddInventory(Inventory, false);
		}

		OnRep_InventoryList();
		MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryManagerComponent, InventoryList, this);
	}
}

void UInventoryManagerComponent::BindToPlayerClass(ACorePlayerState* PlayerState)
{
	PlayerState->OnPlayerClassChanged.AddDynamic(this, &UInventoryManagerComponent::OnPlayerClassChanged);

	if (UPlayerClassComponent* PlayerClassComponent = PlayerState->GetPlayerClassComponent())
	{
		OnPlayerClassChanged(PlayerState, PlayerClassComponent);
	}
}

void UInventoryManagerComponent::OnPlayerClassChanged(ACorePlayerState* PlayerState, UPlayerClassComponent* PlayerClassComponent)
{

}

bool UInventoryManagerComponent::AddInventory(TSubclassOf<UInventory> InventoryClass, bool bDispatchOnRep)
{
	if (!InventoryClass)
	{
		return false;
	}

	for (UInventory* Inventory : InventoryList)
	{
		if (!Inventory)
		{
			continue;
		}

		if (Inventory->GetClass() == InventoryClass)
		{
			return false;
		}
	}

	UInventory* Inventory = NewObject<UInventory>(GetOwner(), InventoryClass);

	if (Inventory)
	{
		InventoryList.Add(Inventory);
		Inventory->RegisterComponent();
		GetOwner()->ForceNetUpdate();
		if (bDispatchOnRep)
		{
			OnRep_InventoryList();
			MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryManagerComponent, InventoryList, this);
		}
	}

	return true;
}

bool UInventoryManagerComponent::RemoveInventory(TSubclassOf<UInventory> InventoryClass, bool bDispatchOnRep)
{
	if (!InventoryClass)
	{
		return false;
	}

	for (UInventory* Inventory : InventoryList)
	{
		if (!Inventory || Inventory->GetClass() == InventoryClass)
		{
			continue;
		}

		Inventory->DestroyComponent();
		if (bDispatchOnRep)
		{
			OnRep_InventoryList();
			MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryManagerComponent, InventoryList, this);
			GetOwner()->ForceNetUpdate();
		}
		return true;
	}

	return false;
}

#define BIND_NEXT_WEAPON(Type)\
InputComponent->BindAction("Next" #Type "Weapon", EInputEvent::IE_Pressed, this, &UInventoryManagerComponent::EquipNextWeapon<EWeaponGroup::Type>); \

#define BIND_WEAPON_FIRE(Type)\
InputComponent->BindAction(#Type "Fire", EInputEvent::IE_Pressed, this, &UInventoryManagerComponent::StartFire<EFireMode::Type>);\
InputComponent->BindAction(#Type "Fire", EInputEvent::IE_Released, this, &UInventoryManagerComponent::StopFire<EFireMode::Type>);\


#define BIND_WEAPON_RELOAD(Type)\
InputComponent->BindAction(#Type "Reload", EInputEvent::IE_Pressed, this, &UInventoryManagerComponent::StartReload<EFireMode::Type>);\
InputComponent->BindAction(#Type "ReloadCancel", EInputEvent::IE_Pressed, this, &UInventoryManagerComponent::StopReload<EFireMode::Type>);\

void UInventoryManagerComponent::SetupInputComponent(UInputComponent* InputComponent)
{
	CurrentInputComponent = InputComponent;

	check(InputComponent);

	BIND_NEXT_WEAPON(Melee);
	BIND_NEXT_WEAPON(Pistol);
	BIND_NEXT_WEAPON(SMG);
	BIND_NEXT_WEAPON(Rifle);
	BIND_NEXT_WEAPON(Special);
	BIND_NEXT_WEAPON(Utility);

	BIND_WEAPON_FIRE(Primary);
	BIND_WEAPON_FIRE(Secondary);
	BIND_WEAPON_FIRE(Tertiary);
	BIND_WEAPON_FIRE(Quaternary);
	BIND_WEAPON_FIRE(Quinary);

	BIND_WEAPON_RELOAD(Primary);
	BIND_WEAPON_RELOAD(Secondary);
	BIND_WEAPON_RELOAD(Tertiary);
	BIND_WEAPON_RELOAD(Quaternary);
	BIND_WEAPON_RELOAD(Quinary);
}

void UInventoryManagerComponent::EquipNextWeapon(EWeaponGroup Group)
{
	if (IsEquipNextWeaponCategoryBlocked(Group))
	{
		return;
	}

	if (!WeaponGroupMap.Contains(Group) || WeaponGroupMap[Group].WeaponArray.Num() == 0)
	{
		return;
	}

	FWeaponGroupArray& WeaponArray = WeaponGroupMap[Group];

	const UWeapon* ComparedWeapon = GetPendingWeapon() ? GetPendingWeapon() : GetCurrentWeapon();

	bool bFoundCurrentWeapon = false;
	for (TWeakObjectPtr<UWeapon> Weapon : WeaponArray)
	{
		if (bFoundCurrentWeapon)
		{
			SetCurrentWeapon(Weapon.Get());
			return;
		}

		if (!Weapon.IsValid())
		{
			continue;
		}

		if (Weapon != ComparedWeapon)
		{
			continue;
		}

		bFoundCurrentWeapon = true;
	}

	SetCurrentWeapon(WeaponArray.WeaponArray[0].Get());
}

void UInventoryManagerComponent::SetCurrentWeapon(UWeapon* DesiredWeapon)
{
	if (!DesiredWeapon)
	{
		return;
	}

	if (IsSimulatedProxy())
	{
		if (DesiredWeapon != GetCurrentWeapon())
		{
			SetPendingWeapon(DesiredWeapon);
			ChangedWeapon();
		}
		return;
	}

	//We are in some sort of state that forbids weapon switching (or forbids switching to this specific weapon).
	if (!CanEquipWeapon(DesiredWeapon))
	{
		return;
	}

	if (!DesiredWeapon->CanEquip())
	{
		//Handle this condition where we cannot equip because we're already equipping (this specific case will fail UWeapon::CanEquip)
		if (GetCurrentWeapon() == DesiredWeapon && DesiredWeapon->IsEquipping())
		{
			SetPendingWeapon(nullptr);
			GetCurrentWeapon()->ClearPendingPutDown();
		}
		return;
	}

	//If we are already holding the desired weapon and have not entered a put down state, clear any pending put down and ignore the weapon switch.
	if (GetCurrentWeapon() == DesiredWeapon && !GetCurrentWeapon()->IsPuttingDown())
	{
		SetPendingWeapon(nullptr);
		GetCurrentWeapon()->ClearPendingPutDown();
		return;
	}

	SetPendingWeapon(DesiredWeapon);

	//If the desired weapon is currently held weapon, and the weapon is not in the inactive state, it is likely the player has performed some sort of duplicate action and so this can be ignored.
	if (GetCurrentWeapon() == DesiredWeapon && !GetCurrentWeapon()->IsInactive())
	{
		return;
	}

	//If we do not have a current weapon, immediately complete call ChangedWeapon to start the desired weapon's equip.
	if (!GetCurrentWeapon())
	{
		if (IsLocallyOwnedRemote())
		{
			Server_Reliable_SetCurrentWeapon(DesiredWeapon);
		}

		ChangedWeapon();
		return;
	}

	//If all of the above are not the case, request the current weapon to be put down (if it is not already). If PutDown fails, assume the weapon will callback here when it's ready to be put down.
	if (!GetCurrentWeapon()->IsPuttingDown())
	{
		if (GetCurrentWeapon()->PutDown())
		{
			if (IsLocallyOwnedRemote())
			{
				Server_Reliable_SetCurrentWeapon(DesiredWeapon);
			}
		}
	}
}

void UInventoryManagerComponent::SetPendingWeapon(UWeapon* DesiredWeapon)
{
	PendingWeapon = DesiredWeapon;
	OnPendingWeaponUpdate.Broadcast(PendingWeapon);
}

void UInventoryManagerComponent::StartFire(EFireMode FireMode)
{
	//Ignore fire inputs if we're hovering a world widget.
	if (GetOwningCharacter() && GetOwningCharacter()->IsHoveringWorldWidget())
	{
		return;
	}

	CurrentlyHeldFireSet.Add(FireMode);

	if (GetCurrentWeapon())
	{
		GetCurrentWeapon()->Fire(FireMode);
	}
}

void UInventoryManagerComponent::StopFire(EFireMode FireMode)
{
	//There seems to be a way to call stop fire without ever calling start fire (input focus issue?). Handle that here.
	if (!CurrentlyHeldFireSet.Contains(FireMode))
	{
		return;
	}

	CurrentlyHeldFireSet.Remove(FireMode);

	if (GetCurrentWeapon())
	{
		GetCurrentWeapon()->StopFire(FireMode);
	}
}

void UInventoryManagerComponent::StartReload(EFireMode FireMode)
{
	if (GetCurrentWeapon())
	{
		GetCurrentWeapon()->Reload(FireMode);
	}
}

void UInventoryManagerComponent::StopReload(EFireMode FireMode)
{
	if (GetCurrentWeapon())
	{
		GetCurrentWeapon()->StopReload(FireMode);
	}
}

float UInventoryManagerComponent::GetMovementSpeedModifier() const
{
	if (!bUpdateMovementSpeedModifier)
	{
		return CachedMovementSpeedModifier;
	}
	
	UE_LOG(LogInventoryManager, VeryVerbose, TEXT("%f: Character %s UInventoryManagerComponent::GetMovementSpeedModifier recalculating movement speed modifier."),
		GetWorld()->GetTimeSeconds(),
		*GetNameSafe(GetOwningCharacter()));

	float MovementSpeedModifier = 1.f;

	for (UInventory* Inventory : InventoryList)
	{
		if (!Inventory)
		{
			continue;
		}

		MovementSpeedModifier *= Inventory->GetMovementSpeedModifier();
	}

	bUpdateMovementSpeedModifier = false;
	CachedMovementSpeedModifier = MovementSpeedModifier;
	return CachedMovementSpeedModifier;
}

UWeapon* UInventoryManagerComponent::GetNextBestWeapon() const
{
	float Rating = -MAX_FLT;
	UWeapon* BestWeapon = nullptr;

	for (UInventory* Inventory : InventoryList)
	{
		UWeapon* Weapon = Cast<UWeapon>(Inventory);

		if (!Weapon || Weapon == GetCurrentWeapon())
		{
			continue;
		}

		if (!Weapon->CanEquip())
		{
			continue;
		}

		if (Rating > Weapon->GetWeaponRating())
		{
			continue;
		}
		
		BestWeapon = Weapon;
	}

	return BestWeapon;
}

bool UInventoryManagerComponent::ContainsInventory(UInventory* Inventory) const
{
	return InventoryList.Contains(Inventory);
}

bool UInventoryManagerComponent::CanEquipWeapon(UWeapon* Weapon) const
{
	if (IsLocallyOwned() && IsActionBlocked())
	{
		return false;
	}

	return ContainsInventory(Weapon);
}

bool UInventoryManagerComponent::IsActionBlocked() const
{
	//Only the local owner should listen to status components blocking action. Ensures no desyncs.
	if (IsLocallyOwned() && OwningStatusComponent.IsValid() && OwningStatusComponent->IsBlockingAction())
	{
		return true;
	}

	return false;
}

bool UInventoryManagerComponent::IsEquipNextWeaponCategoryBlocked(EWeaponGroup WeaponGroup) const
{
	if (!IsLocallyOwned() || !GetVoiceCommandComponent() || !GetVoiceCommandComponent()->IsVoiceCommandMenuBlockingInput())
	{
		return false;
	}

	TArray<FName> KeyName;
	switch (WeaponGroup)
	{
	case EWeaponGroup::Melee:
		KeyName.Add("NextMeleeWeapon");
		break;
	case EWeaponGroup::Pistol:
		KeyName.Add("NextPistolWeapon");
		break;
	case EWeaponGroup::SMG:
		KeyName.Add("NextSMGWeapon");
		break;
	case EWeaponGroup::Rifle:
		KeyName.Add("NextRifleWeapon");
		break;
	case EWeaponGroup::Special:
		KeyName.Add("NextSpecialWeapon");
		break;
	case EWeaponGroup::Utility:
		KeyName.Add("NextUtilityWeapon");
		break;
	}

	if (GetVoiceCommandComponent()->IsVoiceCommandMenuSharingKeyBindingsWithActions(KeyName))
	{
		return true;
	}

	return false;
}

void UInventoryManagerComponent::GetArrayFromWeaponGroup(const FWeaponGroupArray& WeaponGroup, TArray<UWeapon*>& Array)
{
	Array.Reserve(WeaponGroup.WeaponArray.Num());

	for (TWeakObjectPtr<UWeapon> Weapon : WeaponGroup)
	{
		if (!Weapon.IsValid())
		{
			continue;
		}

		Array.Add(Weapon.Get());
	}

	Array.Shrink();
}

static FText WeaponMeleeCategory = LOCTABLE("/Game/Localization/InventoryStringTable.InventoryStringTable", "Category_Melee");
static FText WeaponPistolCategory = LOCTABLE("/Game/Localization/InventoryStringTable.InventoryStringTable", "Category_Pistol");
static FText WeaponSMGCategory = LOCTABLE("/Game/Localization/InventoryStringTable.InventoryStringTable", "Category_SMG");
static FText WeaponRifleCategory = LOCTABLE("/Game/Localization/InventoryStringTable.InventoryStringTable", "Category_Rifle");
static FText WeaponSpecialCategory = LOCTABLE("/Game/Localization/InventoryStringTable.InventoryStringTable", "Category_Special");
static FText WeaponUtilityCategory = LOCTABLE("/Game/Localization/InventoryStringTable.InventoryStringTable", "Category_Utility");
static FText WeaponNoneCategory = LOCTABLE("/Game/Localization/InventoryStringTable.InventoryStringTable", "Category_Invalid");
FText UInventoryManagerComponent::GetWeaponGroupTitle(EWeaponGroup WeaponGroup)
{
	switch (WeaponGroup)
	{
	case EWeaponGroup::Melee:
		return WeaponMeleeCategory;
	case EWeaponGroup::Pistol:
		return WeaponPistolCategory;
	case EWeaponGroup::SMG:
		return WeaponSMGCategory;
	case EWeaponGroup::Rifle:
		return WeaponRifleCategory;
	case EWeaponGroup::Special:
		return WeaponSpecialCategory;
	case EWeaponGroup::Utility:
		return WeaponUtilityCategory;
	case EWeaponGroup::None:
		return WeaponNoneCategory;
	}

	return WeaponNoneCategory;
}

void UInventoryManagerComponent::WeaponEquipComplete(UWeapon* Weapon)
{
	if (!Weapon || Weapon != GetCurrentWeapon())
	{
		return;
	}

	if (GetCurrentWeapon()->IsPendingPutDown() && GetPendingWeapon())
	{
		SetCurrentWeapon(GetPendingWeapon());
	}
}

void UInventoryManagerComponent::WeaponPutDownComplete(UWeapon* Weapon)
{
	if (Weapon != GetCurrentWeapon())
	{
		return;
	}

	//Simulated proxy will switch weapons via the replicated variable CurrentWeapon.
	if (!IsSimulatedProxy())
	{
		ChangedWeapon();
	}
}

void UInventoryManagerComponent::ChangedWeapon()
{
	if (!IsSimulatedProxy())
	{
		if (GetCurrentWeapon() && !GetCurrentWeapon()->IsInactive())
		{
			GetCurrentWeapon()->ForcePutDown();

			if (!GetCurrentWeapon()->IsInactive())
			{
				UE_LOG(LogInventoryManager, Error, TEXT("%f: Character %s %s UInventoryManagerComponent::ChangedWeapon could not change weapon because CurrentWeapon %s is not inactive and UWeapon::ForcePutDown did not succeed."),
					GetWorld()->GetTimeSeconds(),
					*GetNameSafe(GetOwningCharacter()), *UCoreGameplayStatics::GetNetRoleNameForActor(GetOwningCharacter()),
					*GetNameSafe(CurrentWeapon));

				LOG_WEAPON_STATE(CurrentWeapon, Display);

				GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UInventoryManagerComponent::ChangedWeapon));
				return;
			}
		}

		if (!PendingWeapon || !PendingWeapon->CanEquip())
		{
			PendingWeapon = GetCurrentWeapon() ? GetCurrentWeapon() : GetNextBestWeapon();

			//This is not good. Maybe ask again next tick or something.
			if (!PendingWeapon || !PendingWeapon->CanEquip())
			{
				UE_LOG(LogInventoryManager, Error, TEXT("%f: Character %s %s UInventoryManagerComponent::ChangedWeapon could not change weapon because PendingWeapon %s was invalid or not equippable."),
					GetWorld()->GetTimeSeconds(),
					*GetNameSafe(GetOwningCharacter()), *UCoreGameplayStatics::GetNetRoleNameForActor(GetOwningCharacter()),
					*GetNameSafe(PendingWeapon));

				LOG_WEAPON_STATE(CurrentWeapon, Display);
				LOG_WEAPON_STATE(PendingWeapon, Display);

				GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UInventoryManagerComponent::ChangedWeapon));
				return;
			}
		}
	}

	UnBindWeaponEvents();

	if (!PendingWeapon)
	{
		UE_LOG(LogInventoryManager, Error, TEXT("%f: Character %s %s UInventoryManagerComponent::ChangedWeapon called but PendingWeapon was nullptr."),
			GetWorld()->GetTimeSeconds(),
			*GetNameSafe(GetOwningCharacter()), *UCoreGameplayStatics::GetNetRoleNameForActor(GetOwningCharacter()));

		LOG_WEAPON_STATE(CurrentWeapon, Display);

		PendingWeapon = CurrentWeapon;
	}

	CurrentWeapon = PendingWeapon;
	SetPendingWeapon(nullptr);

	BindWeaponEvents();

	CurrentWeapon->Equip();

	if (CurrentWeapon->IsInactive())
	{
		UE_LOG(LogInventoryManager, Error, TEXT("%f: Character %s %s UInventoryManagerComponent::ChangedWeapon failed to make CurrentWeapon active."),
			GetWorld()->GetTimeSeconds(),
			*GetNameSafe(GetOwningCharacter()), *UCoreGameplayStatics::GetNetRoleNameForActor(GetOwningCharacter()));

		LOG_WEAPON_STATE(CurrentWeapon, Display);
	}

	for (EFireMode HeldFireMode : CurrentlyHeldFireSet)
	{
		CurrentWeapon->Fire(HeldFireMode);
	}

	OnCurrentWeaponUpdate.Broadcast(CurrentWeapon);

	if (IsLocallyOwnedRemote())
	{
		Server_Reliable_WeaponEquipped(CurrentWeapon);
	}

	if (IsNonOwningAuthority())
	{
		Client_Reliable_CurrentWeaponSet(CurrentWeapon);
	}
}

void UInventoryManagerComponent::OnWeaponChangedCosmetic(UWeapon* Weapon)
{

}

void UInventoryManagerComponent::BindWeaponEvents()
{
	if (!CurrentWeapon)
	{
		return;
	}

	if (CurrentWeapon->OnWeaponPutDownComplete.IsAlreadyBound(this, &UInventoryManagerComponent::WeaponPutDownComplete))
	{
		return;
	}

	CurrentWeapon->OnWeaponEquipComplete.AddDynamic(this, &UInventoryManagerComponent::WeaponEquipComplete);
	CurrentWeapon->OnWeaponPutDownComplete.AddDynamic(this, &UInventoryManagerComponent::WeaponPutDownComplete);
	CurrentWeapon->OnWeaponAwaitingPutDown.AddDynamic(this, &UInventoryManagerComponent::WeaponAwaitingPutDown);
}

void UInventoryManagerComponent::UnBindWeaponEvents()
{
	if (!CurrentWeapon)
	{
		return;
	}

	if (!CurrentWeapon->OnWeaponPutDownComplete.IsAlreadyBound(this, &UInventoryManagerComponent::WeaponPutDownComplete))
	{
		return;
	}

	CurrentWeapon->OnWeaponEquipComplete.RemoveDynamic(this, &UInventoryManagerComponent::WeaponEquipComplete);
	CurrentWeapon->OnWeaponPutDownComplete.RemoveDynamic(this, &UInventoryManagerComponent::WeaponPutDownComplete);
	CurrentWeapon->OnWeaponAwaitingPutDown.RemoveDynamic(this, &UInventoryManagerComponent::WeaponAwaitingPutDown);
}

bool UInventoryManagerComponent::Server_Reliable_SetCurrentWeapon_Validate(UWeapon* DesiredWeapon)
{
	return true;
}

void UInventoryManagerComponent::Server_Reliable_SetCurrentWeapon_Implementation(UWeapon* DesiredWeapon)
{
	if (!DesiredWeapon)
	{
		return;
	}

	SetCurrentWeapon(DesiredWeapon);
}

bool UInventoryManagerComponent::Server_Reliable_WeaponEquipped_Validate(UWeapon* DesiredWeapon)
{
	return true;
}

void UInventoryManagerComponent::Server_Reliable_WeaponEquipped_Implementation(UWeapon* DesiredWeapon)
{
	if (!DesiredWeapon || !CanEquipWeapon(DesiredWeapon))
	{
		return;
	}

	if (!GetCurrentWeapon())
	{
		//If there is no current weapon, run through SetCurrentWeapon immediately.
		SetCurrentWeapon(DesiredWeapon);
		return;
	}

	if (GetCurrentWeapon() == DesiredWeapon)
	{
		//If current weapon is the desired one but is pending a put down, do what we can to abort that.
		if (GetCurrentWeapon()->IsPuttingDown())
		{
			UE_LOG(LogInventoryManager, Display, TEXT("%f: Character %s UInventoryManagerComponent::Server_Reliable_WeaponEquipped detected desync. DesiredWeapon %s was already current but was being put down."),
				GetWorld()->GetTimeSeconds(),
				*GetNameSafe(GetOwningCharacter()),
				*GetNameSafe(DesiredWeapon));

			LOG_WEAPON_STATE(DesiredWeapon, Display);
			LOG_WEAPON_STATE(CurrentWeapon, Display);

			SetPendingWeapon(DesiredWeapon);
			GetCurrentWeapon()->ForcePutDown();
		}
		//If current weapon is inactive but the autonomous proxy has equipped it, force any active weapons to be put down as they are currently desynced.
		else if (GetCurrentWeapon()->IsInactive())
		{
			UE_LOG(LogInventoryManager, Display, TEXT("%f: Character %s UInventoryManagerComponent::Server_Reliable_WeaponEquipped detected desync. DesiredWeapon %s was already current but was inactive."),
				GetWorld()->GetTimeSeconds(),
				*GetNameSafe(GetOwningCharacter()),
				*GetNameSafe(DesiredWeapon));

			LOG_WEAPON_STATE(DesiredWeapon, Display);
			LOG_WEAPON_STATE(CurrentWeapon, Display);

			SetPendingWeapon(DesiredWeapon);
			for (UInventory* Inventory : InventoryList)
			{
				UWeapon* Weapon = Cast<UWeapon>(Inventory);

				if (!Weapon)
				{
					continue;
				}

				Weapon->ForcePutDown();
			}

			//If the above code did not push a weapon equip even, do so now.
			if (!GetCurrentWeapon()->IsEquipping())
			{
				GetCurrentWeapon()->Equip();
			}
		}

		if (DesiredWeapon != GetCurrentWeapon() || (!GetCurrentWeapon()->IsEquipping() && !GetCurrentWeapon()->IsActiveWeapon()))
		{
			UE_LOG(LogInventoryManager, Error, TEXT("%f: Character %s UInventoryManagerComponent::Server_Reliable_WeaponEquipped attempted to correct CurrentWeapon %s inactive desync but failed."),
				GetWorld()->GetTimeSeconds(),
				*GetNameSafe(GetOwningCharacter()),
				*GetNameSafe(CurrentWeapon));

			LOG_WEAPON_STATE(CurrentWeapon, Display);
		}

		return;
	}

	//Attempt to play catch up with the client if we're still holding onto an older weapon.
	if (GetCurrentWeapon() != DesiredWeapon)
	{
		SetCurrentWeapon(DesiredWeapon);

		if (GetCurrentWeapon() != DesiredWeapon)
		{
			UE_LOG(LogInventoryManager, Display, TEXT("%f: Character %s UInventoryManagerComponent::Server_Reliable_WeaponEquipped server is behind client. Forcing put down on CurrentWeapon %s."),
				GetWorld()->GetTimeSeconds(),
				*GetNameSafe(GetOwningCharacter()),
				*GetNameSafe(GetCurrentWeapon()));

			LOG_WEAPON_STATE(DesiredWeapon, Display);
			LOG_WEAPON_STATE(CurrentWeapon, Display);

			SetPendingWeapon(DesiredWeapon);
			GetCurrentWeapon()->ForcePutDown();
			return;
		}
	}
}

void UInventoryManagerComponent::WeaponAwaitingPutDown(UWeapon* Weapon)
{
	//Technically some of these were checked in UWeapon::CheckPendingPutDown() but it's good to make sure.
	if (!IsLocallyOwned())
	{
		return;
	}

	if (!GetPendingWeapon())
	{
		return;
	}

	if (!GetCurrentWeapon()->CanPutDown())
	{
		return;
	}

	SetCurrentWeapon(GetPendingWeapon());
}

void UInventoryManagerComponent::Client_Reliable_CurrentWeaponSet_Implementation(UWeapon* Weapon)
{
	CurrentServerWeapon = Weapon;

	if (CurrentServerWeapon != CurrentWeapon)
	{
		CheckWeaponSynchronization();
	}
}

void UInventoryManagerComponent::CheckWeaponSynchronization()
{
	if (CurrentServerWeapon == CurrentWeapon)
	{
		UE_LOG(LogInventoryManager, Error, TEXT("%f: Character %s desync has resolved on client. CurrentWeapon: %s CurrentServerWeapon: %s"),
			GetWorld()->GetTimeSeconds(),
			*GetNameSafe(GetOwningCharacter()),
			*GetNameSafe(CurrentWeapon),
			*GetNameSafe(CurrentServerWeapon));
		return;
	}

	UE_LOG(LogInventoryManager, Error, TEXT("%f: Character %s desync detected on client. CurrentWeapon: %s CurrentServerWeapon: %s"),
		GetWorld()->GetTimeSeconds(),
		*GetNameSafe(GetOwningCharacter()),
		*GetNameSafe(CurrentWeapon),
		*GetNameSafe(CurrentServerWeapon));

	LOG_WEAPON_STATE(CurrentWeapon, Log);
	LOG_WEAPON_STATE(CurrentServerWeapon, Log);

	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UInventoryManagerComponent::CheckWeaponSynchronization));
}

void UInventoryManagerComponent::OnRep_InventoryList()
{
	if (!bHasEquipedInitialWeapon)
	{
		EquipInitialWeapon();
	}

	if (PreviousInventoryList.Num() > InventoryList.Num())
	{
		//Our currently equipped weapon is going through some sort of garbage collection
		if (!CurrentWeapon || !CurrentWeapon->IsPendingKill() || !CurrentWeapon->IsBeingDestroyed() || !InventoryList.Contains(CurrentWeapon))
		{
			SetCurrentWeapon(GetPendingWeapon() ? GetPendingWeapon() : GetNextBestWeapon());
		}
	}

	for (UInventory* Inventory : InventoryList)
	{
		if (!Inventory || Inventory->IsPendingKill())
		{
			continue;
		}

		if (!PreviousInventoryList.Contains(Inventory))
		{
			if (Inventory->OnComponentEndPlay.IsAlreadyBound(this, &UInventoryManagerComponent::OnInventoryEndPlay))
			{
				Inventory->OnComponentEndPlay.AddDynamic(this, &UInventoryManagerComponent::OnInventoryEndPlay);
			}

			OnInventoryAdded.Broadcast(Inventory);
		}
	}

	RequestMovementSpeedModifierUpdate();

	if (IsLocallyOwned())
	{
		UpdateWeaponGroupMap();
	}

	OnInventoryUpdate.Broadcast(this);
	PreviousInventoryList = InventoryList;
}

void UInventoryManagerComponent::EquipInitialWeapon()
{
	if (IsSimulatedProxy() || GetCurrentWeapon())
	{
		bHasEquipedInitialWeapon = true;
		return;
	}

	if (!GetOwningCharacter() || !GetOwningCharacter()->GetController())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UInventoryManagerComponent::EquipInitialWeapon));
		return;
	}

	if (!IsLocallyOwned())
	{
		return;
	}

	//First pass iteration to check if we're missing any inventory items.
	for (UInventory* Inventory : InventoryList)
	{
		if (!Inventory || !Inventory->HasBegunPlay())
		{
			GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UInventoryManagerComponent::EquipInitialWeapon));
			return;
		}
	}
	
	UWeapon* HighestPriorityWeapon = nullptr;
	uint8 HighestPriority = 0;
	bool bHighestAvoidsInitial = false;
	UWeapon* Weapon = nullptr;

	for (UInventory* Inventory : InventoryList)
	{
		Weapon = Cast<UWeapon>(Inventory);

		if (!Weapon || !Weapon->CanEquip())
		{
			continue;
		}

		if (!HighestPriorityWeapon || (bHighestAvoidsInitial && !Weapon->ShouldAvoidInitialEquip()) || HighestPriority < Weapon->GetWeaponPriority())
		{
			HighestPriority = Weapon->GetWeaponPriority();
			bHighestAvoidsInitial = Weapon->ShouldAvoidInitialEquip();
			HighestPriorityWeapon = Weapon;
		}
	}

	if (HighestPriorityWeapon)
	{
		SetCurrentWeapon(HighestPriorityWeapon);
		bHasEquipedInitialWeapon = GetCurrentWeapon() != nullptr;
		return;
	}

	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UInventoryManagerComponent::EquipInitialWeapon));
}

void UInventoryManagerComponent::UpdateWeaponGroupMap()
{
	if (!IsLocallyOwned())
	{
		return;
	}

	TArray<EWeaponGroup> GroupUpdateList;

	TMap<EWeaponGroup, FWeaponGroupArray> NewWeaponGroupMap;
	NewWeaponGroupMap.Reserve(MAXWEAPONGROUP);

	for (UInventory* Inventory : InventoryList)
	{
		UWeapon* Weapon = Cast<UWeapon>(Inventory);
		EWeaponGroup Group = Weapon ? Weapon->GetWeaponGroup() : EWeaponGroup::None;

		if (Group == EWeaponGroup::None)
		{
			continue;
		}

		NewWeaponGroupMap.FindOrAdd(Group).WeaponArray.Add(TWeakObjectPtr<UWeapon>(Weapon));
	}

	for (TPair<EWeaponGroup, FWeaponGroupArray>& GroupEntry : WeaponGroupMap)
	{
		if (!NewWeaponGroupMap.Contains(GroupEntry.Key))
		{
			GroupUpdateList.Add(GroupEntry.Key);
			continue;
		}

		bool bFoundNullWeapon = false;
		for (TWeakObjectPtr<UWeapon> Weapon : GroupEntry.Value)
		{
			if (!Weapon.IsValid())
			{
				bFoundNullWeapon = true;
				break;
			}
		}

		if (bFoundNullWeapon)
		{
			GroupUpdateList.Add(GroupEntry.Key);
			continue;
		}

		bool bFoundNewWeapon = false;
		const TArray<TWeakObjectPtr<UWeapon>>& NewWeaponList = NewWeaponGroupMap[GroupEntry.Key].WeaponArray;

		for (const TWeakObjectPtr<UWeapon>& Weapon : NewWeaponList)
		{
			if (!GroupEntry.Value.WeaponArray.Contains(Weapon))
			{
				bFoundNewWeapon = true;
				break;
			}
		}

		if (bFoundNewWeapon)
		{
			GroupUpdateList.Add(GroupEntry.Key);
			continue;
		}
	}

	for (TPair<EWeaponGroup, FWeaponGroupArray>& GroupEntry : NewWeaponGroupMap)
	{
		if (!WeaponGroupMap.Contains(GroupEntry.Key))
		{
			GroupUpdateList.Add(GroupEntry.Key);
			continue;
		}
	}
	
	for (const EWeaponGroup& Group : GroupUpdateList)
	{
		if (WeaponGroupMap.Contains(Group))
		{
			WeaponGroupMap[Group].WeaponArray.Sort(UWeapon::FSortByPriority());
		}
	}

	WeaponGroupMap = MoveTemp(NewWeaponGroupMap);

	for (const EWeaponGroup& Group : GroupUpdateList)
	{
		OnInventoryGroupUpdate.Broadcast(Group);
	}
}

void UInventoryManagerComponent::OnInventoryEndPlay(UCoreCharacterComponent* Component, EEndPlayReason::Type Reason)
{
	UInventory* Inventory = Cast<UInventory>(Component);

	if (!Inventory)
	{
		return;
	}

	OnInventoryRemoved.Broadcast(Inventory);
}

void UInventoryManagerComponent::OnCharacterDied(UStatusComponent* Component, float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (ANauseaPlayerState* NauseaPlayerState = Cast<ANauseaPlayerState>(GetOwningCharacter()->GetOwningPlayerState()))
	{
		NauseaPlayerState->OnPlayerClassChanged.RemoveDynamic(this, &UInventoryManagerComponent::OnPlayerClassChanged);
	}
}

void UInventoryManagerComponent::OnActionInterrupt(const UStatusComponent* Component)
{
	if (!IsLocallyOwned() || !GetCurrentWeapon())
	{
		return;
	}

	GetCurrentWeapon()->StopAllFire();
	GetCurrentWeapon()->StopAllReload();
}