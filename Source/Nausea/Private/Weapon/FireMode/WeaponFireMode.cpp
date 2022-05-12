// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#include "Weapon/FireMode/WeaponFireMode.h"
#include "Engine/ActorChannel.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/GameStateBase.h"
#include "System/NetHelper.h"
#include "Character/CoreCharacter.h"
#include "Player/PlayerClassComponent.h"
#include "Weapon/Weapon.h"
#include "Weapon/FireMode/Ammo.h"
#include "Character/CoreCharacterAnimInstanceTypes.h"
#include "Player/CameraModifier/RecoilCameraModifier.h"

UWeaponFireMode::UWeaponFireMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCanEverTick = true;
}

void UWeaponFireMode::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(UWeaponFireMode, Ammo, PushReplicationParams::Default);
}

void UWeaponFireMode::Tick(float DeltaTime)
{
	if (CurrentSpread > 0.f)
	{
		CurrentSpreadDecayRate += DeltaTime * SpreadDecayRate;
		CurrentSpread = FMath::Max(CurrentSpread - (DeltaTime * CurrentSpreadDecayRate), 0.f);
		OnFireModeSpreadUpdate.Broadcast(this, GetFireSpread());
	}
	
	if (bTickEnabled)
	{
		Super::Tick(DeltaTime);
	}
}

void UWeaponFireMode::RegisterOwningWeaponClass(const UWeapon* Weapon)
{
	Super::RegisterOwningWeaponClass(Weapon);

	ensure(GetOwningWeaponClass());

	//Maybe register damage types?
}

void UWeaponFireMode::Initialize(UWeapon* Weapon)
{
	if (GetOwningCharacter())
	{
		ensure(Weapon == GetOwningWeapon());
		return;
	}

	Super::Initialize(Weapon);

	if (GetAmmo())
	{
		GetAmmo()->UpdateAmmoCapacity(true);
		MARK_PROPERTY_DIRTY_FROM_NAME(UWeaponFireMode, Ammo, this);
	}

	if (GetOwningWeapon()->IsLocallyOwned() && SpreadPercentIncreasePerShot > 0.f)
	{
		TickType = ETickableTickType::Conditional;
	}
}

bool UWeaponFireMode::IsFiring_Implementation() const
{
	return GetWorld()->GetTimerManager().IsTimerActive(FireTimerHandle);
}

bool UWeaponFireMode::CanPutDown() const
{
	if (!Super::CanPutDown())
	{
		return false;
	}

	if (GetAmmo() && !GetAmmo()->CanPutDown())
	{
		return false;
	}

	return true;
}

bool UWeaponFireMode::BlockAction(const UFireMode* InstigatorFireMode) const
{
	if (Super::BlockAction(InstigatorFireMode))
	{
		return true;
	}

	if (GetAmmo() && GetAmmo()->BlockAction(InstigatorFireMode))
	{
		return true;
	}

	return false;
}

bool UWeaponFireMode::CanFire() const
{
	if (!Super::CanFire())
	{
		return false;
	}

	if (GetOwningWeapon()->IsSimulatedProxy())
	{
		return true;
	}

	if (!CanConsumeAmmo())
	{
		return false;
	}

	return true;
}

bool UWeaponFireMode::Fire(float WorldTimeOverride)
{
	if (!Super::Fire(WorldTimeOverride))
	{
		return false;
	}
	
	//If we need to catch up on non owning authority, do so now.
	if (GetOwningWeapon()->IsNonOwningAuthority() && GetAmmo() && GetAmmo()->IsReloading())
	{
		GetAmmo()->ForceReloadComplete();
	}
	
	if (IsLocallyOwned() && GetAmmo() && GetAmmo()->IsReloading())
	{
		StopReload();
	}

	if (!ConsumeAmmo())
	{
		return false;
	}

	if (GetOwningWeapon()->IsLocallyOwned())
	{
		if (FireType != EFireType::Automatic)
		{
			ClearHoldingFire();
		}

		if (SpreadPercentIncreasePerShot > 0.f)
		{
			CurrentSpread = FMath::Min(CurrentSpread + SpreadPercentIncreasePerShot, 1.f);
			CurrentSpreadDecayRate = 0.f;
			SetTickEnabled(true);
			OnFireModeSpreadUpdate.Broadcast(this, GetFireSpread());
		}
	}

	float FireDuration = GetFireRate();

	if (WorldTimeOverride != -1.f)
	{
		WorldTimeOverride = FMath::Min(GetWorld()->GetGameState()->GetServerWorldTimeSeconds(), WorldTimeOverride);
		FireDuration = FMath::Max3(FireDuration - (GetWorld()->GetGameState()->GetServerWorldTimeSeconds() - WorldTimeOverride), FireDuration * 0.5f, 0.01f);
	}

	GetWorld()->GetTimerManager().SetTimer(FireTimerHandle, FTimerDelegate::CreateUObject(this, &UWeaponFireMode::FireComplete), GetOwningWeapon()->IsNonOwningAuthority() ? FireDuration * 0.66f : FireDuration, false);
	return true;
}

void UWeaponFireMode::StopFire(float WorldTimeOverride)
{
	Super::StopFire(WorldTimeOverride);
}

bool UWeaponFireMode::Reload()
{
	if (!GetWorld() || !GetWorld()->GetGameState())
	{
		return false;
	}

	if (!GetAmmo() || !GetAmmo()->Reload())
	{
		return false;
	}

	return true;
}

bool UWeaponFireMode::StopReload()
{
	if (!GetWorld() || !GetWorld()->GetGameState())
	{
		return false;
	}

	if (!GetAmmo())
	{
		return false;
	}

	GetAmmo()->StopReload(GetWorld()->GetGameState()->GetServerWorldTimeSeconds());

	return true;
}

void UWeaponFireMode::OnFireCosmetic()
{
	if (!GetOwningWeapon())
	{
		return;
	}

	ACoreCharacter* Character = GetOwningCharacter();

	if (!Character)
	{
		return;
	}

	if (const UAnimationObject* AnimationObject1P = GetOwningCharacter()->GetFirstPersonAnimObject())
	{
		AnimationObject1P->PlayFireMontage(Character->GetMesh1P(), Character->GetWeaponMesh1P(), GetFireRate(), GetFireMode());
	}

	if (const UAnimationObject* AnimationObject3P = GetOwningCharacter()->GetThirdPersonAnimObject())
	{
		AnimationObject3P->PlayFireMontage(Character->GetMesh(), Character->GetWeaponMesh(), GetFireRate(), GetFireMode());
	}

	ApplyRecoil();
}

void UWeaponFireMode::FireComplete()
{
	GetWorld()->GetTimerManager().ClearTimer(FireTimerHandle);

	Super::FireComplete();

	if (GetOwningWeapon()->IsSimulatedProxy())
	{
		UpdateFireCounter();
		return;
	}

	if (!GetOwningWeapon()->IsLocallyOwned())
	{
		return;
	}

	if (CanRefire())
	{
		Fire();
	}
}

void UWeaponFireMode::BindWeaponEvents()
{
	if (!GetOwningWeapon() || GetOwningWeapon()->OnWeaponEquipComplete.IsAlreadyBound(this, &UWeaponFireMode::WeaponEquipComplete))
	{
		return;
	}

	Super::BindWeaponEvents();

	if (GetAmmo())
	{
		GetAmmo()->OnReloadComplete.AddDynamic(GetOwningWeapon(), &UWeapon::ReloadCompleted);
	}
}

void UWeaponFireMode::UnBindWeaponEvents()
{
	if (!GetOwningWeapon() || GetOwningWeapon()->IsPendingKill() || !GetOwningWeapon()->OnWeaponEquipComplete.IsAlreadyBound(this, &UWeaponFireMode::WeaponEquipComplete))
	{
		return;
	}

	Super::UnBindWeaponEvents();

	if (GetAmmo())
	{
		GetAmmo()->OnReloadComplete.RemoveDynamic(GetOwningWeapon(), &UWeapon::ReloadCompleted);
	}
}

void UWeaponFireMode::Client_Reliable_FailedFire_Implementation()
{
	if (GetAmmo())
	{
		GetAmmo()->ApplyAmmoCorrection();
	}
}

float UWeaponFireMode::GetFireRate() const
{
	float ModifiedFireRate = FireRate;
	if (UPlayerClassComponent* PlayerClassComponent = GetPlayerClassComponent())
	{
		PlayerClassComponent->OnProcessFireRate.Broadcast(GetOwningWeapon(), this, ModifiedFireRate);
	}

	return ModifiedFireRate;
}

float UWeaponFireMode::GetFireSpread() const
{
	return FMath::Lerp(SpreadRange.X, SpreadRange.Y, CurrentSpread);
}

FVector UWeaponFireMode::GetFireDirection() const
{
	if (UCameraComponent* CameraComponent = GetOwningCharacter() ? GetOwningCharacter()->GetFirstPersonCamera() : nullptr)
	{
		return CameraComponent->GetComponentRotation().Vector();
	}

	return FVector(-MAX_FLT);
}

FVector UWeaponFireMode::GetFireLocation() const
{
	if (UCameraComponent* CameraComponent = GetOwningCharacter() ? GetOwningCharacter()->GetFirstPersonCamera() : nullptr)
	{
		return CameraComponent->GetComponentLocation();
	}

	return FVector(-MAX_FLT);
}

const UAmmo* UWeaponFireMode::GetAmmoDefaultObject() const
{
	if (GetOwningWeapon())
	{
		return GetOwningWeapon()->GetAmmoDefaultObject(GetFireMode());
	}

	if (const UWeapon* OuterWeapon = GetTypedOuter<UWeapon>())
	{
		return OuterWeapon->GetAmmoDefaultObject(GetFireMode());
	}

	return nullptr;
}

bool UWeaponFireMode::CanRefire() const
{
	if (!CanFire())
	{
		return false;
	}

	return IsHoldingFire();
}

bool UWeaponFireMode::CanConsumeAmmo() const
{
	if (!GetAmmo())
	{
		return true;
	}

	return GetAmmo()->CanConsumeAmmo();
}

bool UWeaponFireMode::ConsumeAmmo()
{
	if (!GetAmmo())
	{
		return true;
	}

	return GetAmmo()->ConsumeAmmo();
}

void UWeaponFireMode::ApplyRecoil()
{
	if (!GetOwningCharacter() || !GetOwningCharacter()->IsCurrentlyFirstPersonViewedPawn())
	{
		return;
	}

	const FVector2D RolledRecoil = FVector2D(FMath::RandRange(RecoilYawVariance.X, RecoilYawVariance.Y) * RecoilStrength.X,
		FMath::RandRange(RecoilPitchVariance.X, RecoilPitchVariance.Y) * RecoilStrength.Y);

	URecoilCameraModifier::CreateRecoilRequest(GetOwningCharacter()->GetViewingPlayerController(), RolledRecoil, RecoilDuration);
}

void UWeaponFireMode::UpdateFireCounter()
{
	if (GetOwningWeapon()->IsPuttingDown() || GetOwningWeapon()->IsInactive())
	{
		return;
	}

	if (LocalFireCounter == FireCounter)
	{
		return;
	}

	LocalFireCounter++;
	LocalFireCounter = FMath::Min(LocalFireCounter, FireCounter);

	if (LocalFireCounter != FireCounter)
	{
		const int32 MaxDiff = FMath::CeilToInt((FireRate + 0.5f) / FireRate);
		LocalFireCounter = FMath::Max(FireCounter - MaxDiff, LocalFireCounter);
	}

	Fire();
}