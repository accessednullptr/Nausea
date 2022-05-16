// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Weapon/FireMode.h"
#include "Net/UnrealNetwork.h"
#include "System/CoreGameplayStatics.h"
#include "NauseaGlobalDefines.h"
#include "NauseaNetDefines.h"
#include "Weapon/Weapon.h"
#include "Player/CorePlayerState.h"
#include "Character/CoreCharacter.h"

#if WITH_EDITOR
#include "Editor.h"
extern UNREALED_API UEditorEngine* GEditor;
#endif

DEFINE_LOG_CATEGORY(LogFireMode);

UFireMode::UFireMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetSkipReplicationLogic(ESkipReplicationLogic::None);
}

void UFireMode::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		TickType = ETickableTickType::Never;
		bTickEnabled = false;
		bK2TickImplemented = IS_K2_FUNCTION_IMPLEMENTED(this, K2_Tick);
		return;
	}

#if WITH_EDITOR
	if (GEditor)
	{
		bK2TickImplemented = IS_K2_FUNCTION_IMPLEMENTED(this, K2_Tick);
	}
#endif

	Initialize(GetTypedOuter<UWeapon>());

	if (AActor* OuterActor = GetTypedOuter<AActor>())
	{
		const bool bShouldTick = bCanEverTick && !(bNeverTickOnDedicatedServer && OuterActor->IsNetMode(NM_DedicatedServer));

		TickType = bShouldTick ? ETickableTickType::Conditional : ETickableTickType::Never;
		bTickEnabled = bShouldTick && bStartWithTickEnabled;
	}
}

void UFireMode::BeginDestroy()
{
	UnBindWeaponEvents();
	OwningWeapon = nullptr;
	OwningCharacter = nullptr;
	WorldPrivate = nullptr;
	bTickEnabled = false;

	Super::BeginDestroy();
}

ETickableTickType UFireMode::GetTickableTickType() const
{
	return TickType;
}

ACorePlayerState* UFireMode::GetOwningPlayerState() const
{
	//Fallback for authorities that might be performing this initialization very early.
	if (!GetOwningPawn()->GetPlayerState() && GetOwningController())
	{
		return GetOwningController()->GetPlayerState<ACorePlayerState>();
	}

	return GetOwningPawn()->GetPlayerState<ACorePlayerState>();
}

AController* UFireMode::GetOwningController() const
{
	return GetOwningPawn()->GetController();
}

APawn* UFireMode::GetOwningPawn() const
{
	if (!OwningCharacter)
	{
		return GetTypedOuter<APawn>();
	}

	return OwningCharacter;
}

void UFireMode::RegisterOwningWeaponClass(const UWeapon* Weapon)
{
	if (!ensure(Weapon))
	{
		return;
	}

	if (OwningWeaponClass && OwningWeaponClass != Weapon->GetClass() && !GetNameSafe(Weapon->GetClass()).Equals(GetNameSafe(OwningWeaponClass)))
	{
		UE_LOG(LogFireMode, Warning, TEXT("%s UFireMode::RegisterOwningWeaponClass setting OwningWeaponClass to %s despite already having one set (%s). Can be intentional."),
			*GetNameSafe(GetClass()), *GetNameSafe(Weapon->GetClass()), *GetNameSafe(OwningWeaponClass));
	}

	OwningWeaponClass = Weapon->GetClass();
}

void UFireMode::InitializeFireModeSlot(EFireMode Slot)
{
	if (!ensure(Slot != EFireMode::MAX))
	{
		return;
	}

	FireMode = Slot;
}

void UFireMode::Initialize(UWeapon* Weapon)
{
	if (!ensure(Weapon))
	{
		return;
	}
	
	//Fire modes do not support ownership transferring and as such should never allow a change of ownership. However, remote will receive this property via initial bunch so check some other initializer property.
	if (GetOwningCharacter())
	{
		return;
	}

	if (OwningWeapon && Weapon != OwningWeapon)
	{
		UE_LOG(LogFireMode, Error, TEXT("%f: Character %s %s UFireMode::Initialize failed due to firemode %s trying to set OwningWeapon to %s when it was already set to %s."),
			GetWorld()->GetTimeSeconds(), *GetNameSafe(Weapon->GetOwningCharacter()), *UCoreGameplayStatics::GetNetRoleNameForActor(Weapon->GetOwningCharacter()), *GetName(), *GetNameSafe(Weapon), *GetNameSafe(OwningWeapon));
	}

	OwningWeapon = Weapon;
	WorldPrivate = GetOwningWeapon()->GetWorld();
	OwningCharacter = OwningWeapon->GetTypedOuter<ACoreCharacter>();

	BindWeaponEvents();
}

const UFireMode* UFireMode::GetDefaultObject() const
{
	if (OwningWeapon)
	{
		return OwningWeapon->GetFireModeDefaultObject(FireMode);
	}

	if (const UWeapon* OuterWeapon = GetTypedOuter<UWeapon>())
	{
		return OuterWeapon->GetFireModeDefaultObject(FireMode);
	}

	return nullptr;
}

bool UFireMode::IsFiring_Implementation() const
{
	return false;
}

bool UFireMode::CanPutDown() const
{
	return !BlockAction(nullptr);
}

bool UFireMode::BlockAction(const UFireMode* InstigatorFireMode) const
{
	if (!ShouldBlockOtherFireModeAction() || (InstigatorFireMode && InstigatorFireMode->IgnoresFireModeActionBlock()))
	{
		return false;
	}

	//Other firemodes firing block the current one trying. However, the current firemode firing blocking itself is handled by the individual firemode itself.
	return InstigatorFireMode != this && IsFiring();
}

bool UFireMode::HasAnyDescriptors(int32 Flags) const
{
	return (int32(FireModeDescriptor) & Flags) != 0;
}

bool UFireMode::CanFire() const
{
	if (!GetOwningWeapon())
	{
		UE_LOG(LogFireMode, Verbose, TEXT("%s UFireMode::CanFire returned false because OwningWeapon was nullptr."), *GetNameSafe(this));
		return false;
	}

	if (GetOwningWeapon()->IsSimulatedProxy())
	{
		UE_LOG(LogFireMode, Verbose, TEXT("%s UFireMode::CanFire returned true because OwningWeapon is ROLE_SimulatedProxy."), *GetNameSafe(this));
		return true;
	}

	if (!GetOwningWeapon()->CanFire(this))
	{
		UE_LOG(LogFireMode, Verbose, TEXT("%s UFireMode::CanFire returned false because %s's UWeapon::CanFire() returned false."), *GetNameSafe(this), *GetNameSafe(GetOwningWeapon()));
		return false;
	}

	if (IsFiring())
	{
		UE_LOG(LogFireMode, Verbose, TEXT("%s UFireMode::CanFire returned false because UFireMode::IsFiring() returned true."), *GetNameSafe(this));
		return false;
	}

	bool bCanFire = true;
	K2_CanFire(bCanFire);
	return bCanFire;
}

bool UFireMode::Fire(float WorldTimeOverride)
{
	if (GetOwningWeapon() && GetOwningWeapon()->IsLocallyOwned()
		&& !GetOwningWeapon()->IsPuttingDown() && !GetOwningWeapon()->IsInactive())
	{
		bHoldingFire = true;
	}

	if (!CanFire())
	{
		return false;
	}

	PerformFire(); //Do fire before cosmetic events.
	
	if (!GetOwningWeapon()->IsNetMode(NM_DedicatedServer))
	{
		OnFireCosmetic();
	}

	K2_OnFire();
	OnFireStart.Broadcast(this);

	return true;
}

void UFireMode::StopFire(float WorldTimeOverride)
{
	ClearHoldingFire();

	K2_OnStopFire();
}

void UFireMode::ForceEndFire()
{
	if (!GetOwningWeapon()->IsNonOwningAuthority())
	{
		return;
	}

	if (IsHoldingFire())
	{
		StopFire();
	}

	if (IsFiring())
	{
		FireComplete();
	}
}

void UFireMode::FireComplete()
{
	K2_OnFireComplete();
	OnFireComplete.Broadcast(this);
}

void UFireMode::BindWeaponEvents()
{
	if (!GetOwningWeapon() || GetOwningWeapon()->OnWeaponEquipComplete.IsAlreadyBound(this, &UFireMode::WeaponEquipComplete))
	{
		return;
	}

	GetOwningWeapon()->OnWeaponEquipComplete.AddDynamic(this, &UFireMode::WeaponEquipComplete);
	GetOwningWeapon()->OnWeaponPutDownStart.AddDynamic(this, &UFireMode::WeaponPutDownStart);
	OnFireComplete.AddDynamic(GetOwningWeapon(), &UWeapon::FireCompleted);
}

void UFireMode::UnBindWeaponEvents()
{
	if (!GetOwningWeapon() || GetOwningWeapon()->IsPendingKill() || !GetOwningWeapon()->OnWeaponEquipComplete.IsAlreadyBound(this, &UFireMode::WeaponEquipComplete))
	{
		return;
	}

	GetOwningWeapon()->OnWeaponEquipComplete.RemoveDynamic(this, &UFireMode::WeaponEquipComplete);
	GetOwningWeapon()->OnWeaponPutDownStart.RemoveDynamic(this, &UFireMode::WeaponPutDownStart);
	OnFireComplete.RemoveDynamic(GetOwningWeapon(), &UWeapon::FireCompleted);
}

void UFireMode::WeaponEquipComplete(UWeapon* Weapon)
{
	if (GetOwningWeapon()->IsLocallyOwned() && IsHoldingFire() && CanFire())
	{
		Fire();
	}
}

void UFireMode::WeaponPutDownStart(UWeapon* Weapon)
{
	//...
}

UWorld* UFireMode::GetWorld_Uncached() const
{
	UWorld* ObjectWorld = nullptr;

	const UObject* Owner = GetOwningWeapon();

	if (Owner && !Owner->HasAnyFlags(RF_ClassDefaultObject))
	{
		ObjectWorld = Owner->GetWorld();
	}

	if (ObjectWorld == nullptr)
	{
		if (AActor* Actor = GetTypedOuter<AActor>())
		{
			ObjectWorld = Actor->GetWorld();
		}
		else
		{
			ObjectWorld = Cast<UWorld>(GetOuter());
		}
	}

	return ObjectWorld;
}