// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Weapon/FireMode/LoadedAmmo.h"
#include "GameFramework/GameStateBase.h"
#include "Animation/AnimInstance.h"
#include "Nausea.h"
#include "System/NetHelper.h"
#include "System/CoreGameplayStatics.h"
#include "Character/CoreCharacter.h"
#include "Player/PlayerClassComponent.h"
#include "Character/CoreCharacterAnimInstanceTypes.h"
#include "Weapon/Weapon.h"
#include "Weapon/FireMode/WeaponFireMode.h"

ULoadedAmmo::ULoadedAmmo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void ULoadedAmmo::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(ULoadedAmmo, LoadedAmmoAmount, PushReplicationParams::SkipOwner);
	DOREPLIFETIME_WITH_PARAMS_FAST(ULoadedAmmo, ReloadCounter, PushReplicationParams::SkipOwner);
}

void ULoadedAmmo::Initialize(UFireMode* FireMode)
{
	const bool bAlreadyDidInitialization = bDoneFirstInitialization;

	Super::Initialize(FireMode);

	if (bAlreadyDidInitialization)
	{
		return;
	}

	LoadedAmmoAmount = MaxLoadedAmmoAmount;
	OnRep_LoadedAmmoAmount(-MAX_FLT);
	MARK_PROPERTY_DIRTY_FROM_NAME(ULoadedAmmo, LoadedAmmoAmount, this);
}

bool ULoadedAmmo::ConsumeAmmo(float Amount)
{
	if (!GetOwningFireMode())
	{
		UE_LOG(LogAmmo, VeryVerbose, TEXT("%f: Character %s %s ULoadedAmmo::ConsumeAmmo returned false due to %s's OwningFireMode being nullptr."),
			GetWorld()->GetTimeSeconds(), *GetNameSafe(GetOwningCharacter()), *UCoreGameplayStatics::GetNetRoleNameForActor(GetOwningCharacter()), *GetName());
		return false;
	}

	if (GetOwningWeapon()->IsSimulatedProxy())
	{
		return true;
	}

	if (!CanConsumeAmmo(Amount))
	{
		UE_LOG(LogAmmo, VeryVerbose, TEXT("%f: Character %s %s ULoadedAmmo::ConsumeAmmo returned false due to %s's CanConsumeAmmo returning false."),
			GetWorld()->GetTimeSeconds(), *GetNameSafe(GetOwningCharacter()), *UCoreGameplayStatics::GetNetRoleNameForActor(GetOwningCharacter()), *GetName());
		return false;
	}

	const float PreviousAmount = LoadedAmmoAmount;
	LoadedAmmoAmount -= Amount;
	OnRep_LoadedAmmoAmount(PreviousAmount);
	MARK_PROPERTY_DIRTY_FROM_NAME(ULoadedAmmo, LoadedAmmoAmount, this);
	return true;
}

bool ULoadedAmmo::CanConsumeAmmo(float Amount) const
{
	return Amount <= LoadedAmmoAmount;
}

void ULoadedAmmo::ApplyAmmoCorrection(float Amount)
{
	const float PreviousAmount = LoadedAmmoAmount;
	LoadedAmmoAmount += Amount;
	OnRep_LoadedAmmoAmount(PreviousAmount);
	MARK_PROPERTY_DIRTY_FROM_NAME(ULoadedAmmo, LoadedAmmoAmount, this);
}

bool ULoadedAmmo::CanReload() const
{
	if (!Super::CanReload())
	{
		UE_LOG(LogAmmo, VeryVerbose, TEXT("%f: Character %s %s ULoadedAmmo::CanReload returned false. Super::CanReload returned false."),
			GetWorld()->GetTimeSeconds(), *GetNameSafe(GetOwningCharacter()), *UCoreGameplayStatics::GetNetRoleNameForActor(GetOwningCharacter()), *GetName());
		return false;
	}

	if (GetOwningWeapon()->IsSimulatedProxy())
	{
		return true;
	}

	if (LoadedAmmoAmount >= MaxLoadedAmmoAmount)
	{
		UE_LOG(LogAmmo, VeryVerbose, TEXT("%f: Character %s %s ULoadedAmmo::CanReload returned false. Super::CanReload returned false. LoadedAmmoAmount is at capacity."),
			GetWorld()->GetTimeSeconds(), *GetNameSafe(GetOwningCharacter()), *UCoreGameplayStatics::GetNetRoleNameForActor(GetOwningCharacter()), *GetName());
		return false;
	}

	if (GetAmmoAmount() <= 0.f)
	{
		UE_LOG(LogAmmo, VeryVerbose, TEXT("%f: Character %s %s ULoadedAmmo::CanReload returned false. Super::CanReload returned false. GetAmmoAmount() returned 0.f."),
			GetWorld()->GetTimeSeconds(), *GetNameSafe(GetOwningCharacter()), *UCoreGameplayStatics::GetNetRoleNameForActor(GetOwningCharacter()), *GetName());
		return false;
	}

	return true;
}

bool ULoadedAmmo::Reload(float WorldTimeOverride = -1.f)
{
	if (!CanReload())
	{
		UE_LOG(LogAmmo, Verbose, TEXT("%f: Character %s %s ULoadedAmmo::Reload attempt to reload %s failed. ULoadedAmmo::CanReload returned false."),
			GetWorld()->GetTimeSeconds(), *GetNameSafe(GetOwningCharacter()), *UCoreGameplayStatics::GetNetRoleNameForActor(GetOwningCharacter()), *GetName());
		return false;
	}

	if (GetWorld()->GetGameState())
	{
		ReloadHistory.Insert(FReloadHistory(WorldTimeOverride == -1.f ? GetWorld()->GetGameState()->GetServerWorldTimeSeconds() : WorldTimeOverride, GetReloadAmount()), 0);
		ReloadHistory.SetNum(FMath::Min(5, ReloadHistory.Num()));

		if (GetOwningWeapon()->IsLocallyOwnedRemote())
		{
			Server_Reliable_Reload(GetWorld()->GetGameState()->GetServerWorldTimeSeconds());
		}
	}

	if (GetOwningWeapon()->IsAuthority())
	{
		if (bMakeNoiseOnReload)
		{
			ReloadNoise.MakeNoise(GetOwningCharacter());
		}

		ReloadCounter++;
		MARK_PROPERTY_DIRTY_FROM_NAME(ULoadedAmmo, ReloadCounter, this);
	}

	OnReloadCosmetic();
	OnReloadBegin.Broadcast(this);

	float ReloadDuration = GetReloadRate();

	if (WorldTimeOverride != -1.f)
	{
		WorldTimeOverride = FMath::Min(GetWorld()->GetGameState()->GetServerWorldTimeSeconds(), WorldTimeOverride);
		ReloadDuration = FMath::Max(ReloadDuration - (GetWorld()->GetGameState()->GetServerWorldTimeSeconds() - WorldTimeOverride - 0.1f), GetReloadRate() * 0.5f);
		ReloadDuration = FMath::Max(ReloadDuration, 0.01f);

		UE_LOG(LogAmmo, VeryVerbose, TEXT("%f: Character %s %s ULoadedAmmo::Reload %s has adjusted reload duration from %f to %f on non owning authority."),
			GetWorld()->GetTimeSeconds(), *GetNameSafe(GetOwningCharacter()), *UCoreGameplayStatics::GetNetRoleNameForActor(GetOwningCharacter()), *GetName(), GetReloadRate(), ReloadDuration);
	}

	GetWorld()->GetTimerManager().SetTimer(ReloadTimer, FTimerDelegate::CreateUObject(this, &ULoadedAmmo::ReloadComplete, GetWorld()->GetGameState()->GetServerWorldTimeSeconds()), ReloadDuration, false);
	return true;
}

bool ULoadedAmmo::StopReload(float WorldTimeOverride = -1.f)
{
	if (!IsIndividualReload() || !IsReloadCancelable())
	{
		return false;
	}

	if (!IsReloading() && GetOwningWeapon()->IsNonOwningAuthority())
	{
		int32 FoundIndex = INDEX_NONE;
		for (int32 Index = 0; Index < ReloadHistory.Num(); Index--)
		{
			if (WorldTimeOverride != ReloadHistory[Index].ReloadTimeStamp)
			{
				continue;
			}

			if (ReloadHistory[Index].bHasApplied)
			{
				LoadedAmmoAmount -= ReloadHistory[Index].ReloadAmount;
				AmmoAmount += ReloadHistory[Index].ReloadAmount;
				break;
			}

			ReloadHistory[Index].bHasFailed = true;
			break;
		}
	}

	GetWorld()->GetTimerManager().ClearTimer(ReloadTimer);
	return false;
}

bool ULoadedAmmo::IsReloading() const
{
	return GetWorld()->GetTimerManager().IsTimerActive(ReloadTimer);
}

bool ULoadedAmmo::IsReloadNearlyComplete() const
{
	if (GetOwningWeapon() && GetOwningWeapon()->IsAuthority() && !GetOwningWeapon()->IsLocallyOwned())
	{
		UE_LOG(LogAmmo, VeryVerbose, TEXT("%f: Character %s %s ULoadedAmmo::IsReloadNearlyComplete was called for %s. Reload time left: %f."),
			GetWorld()->GetTimeSeconds(), *GetNameSafe(GetOwningCharacter()), *UCoreGameplayStatics::GetNetRoleNameForActor(GetOwningCharacter()), *GetName(), GetWorld()->GetTimerManager().GetTimerRemaining(ReloadTimer));

		return GetWorld()->GetTimerManager().IsTimerActive(ReloadTimer) && GetWorld()->GetTimerManager().GetTimerRemaining(ReloadTimer) < 0.2f;
	}

	return false;
}

bool ULoadedAmmo::CanPutDown() const
{
	return !BlockAction();
}

bool ULoadedAmmo::BlockAction(const UFireMode* InstigatorFireMode) const
{
	if (IsIndividualReload() && IsReloadCancelable())
	{
		return false;
	}

	if (InstigatorFireMode && InstigatorFireMode->GetOwningWeapon() && InstigatorFireMode->GetOwningWeapon()->IsNonOwningAuthority() && IsReloadNearlyComplete())
	{
		return false;
	}

	return IsReloading();
}

void ULoadedAmmo::OnReloadCosmetic()
{
	if (!GetOwningFireMode())
	{
		return;
	}

	ACoreCharacter* Character = GetOwningFireMode()->GetOwningCharacter();

	if (!Character)
	{
		return;
	}

	if (const UAnimationObject* AnimationObject1P = Character->GetFirstPersonAnimObject())
	{
		AnimationObject1P->PlayReloadMontage(Character->GetMesh1P(), Character->GetWeaponMesh1P(), GetReloadRate(), GetOwningFireMode()->GetFireMode());
	}

	if (const UAnimationObject* AnimationObject3P = Character->GetThirdPersonAnimObject())
	{
		AnimationObject3P->PlayReloadMontage(Character->GetMesh(), Character->GetWeaponMesh(), GetReloadRate(), GetOwningFireMode()->GetFireMode());
	}
}

void ULoadedAmmo::UpdateAmmoCapacity(bool bFirstInitialization)
{
	Super::UpdateAmmoCapacity(bFirstInitialization);
	
	float ModifiedLoadedAmmoCapacity = Cast<ULoadedAmmo>(GetDefaultObject())->MaxLoadedAmmoAmount;
	if (UPlayerClassComponent* PlayerClassComponent = GetOwningFireMode()->GetPlayerClassComponent())
	{
		PlayerClassComponent->OnProcessLoadedAmmoCapacity.Broadcast(GetOwningWeapon(), this, ModifiedLoadedAmmoCapacity);
	}
	MaxLoadedAmmoAmount = FMath::FloorToInt(ModifiedLoadedAmmoCapacity);

	if (!bFirstInitialization)
	{
		return;
	}

	LoadedAmmoAmount = FMath::FloorToInt(MaxLoadedAmmoAmount);
	MARK_PROPERTY_DIRTY_FROM_NAME(ULoadedAmmo, LoadedAmmoAmount, this);
}

void ULoadedAmmo::Client_Reliable_ReloadFailed_Implementation(float WorldTimeOverride)
{
	int32 FoundIndex = INDEX_NONE;
	for (int32 Index = 0; Index < ReloadHistory.Num(); Index--)
	{
		if (WorldTimeOverride != ReloadHistory[Index].ReloadTimeStamp)
		{
			continue;
		}

		if (ReloadHistory[Index].bHasApplied)
		{
			LoadedAmmoAmount -= ReloadHistory[Index].ReloadAmount;
			AmmoAmount += ReloadHistory[Index].ReloadAmount;
			break;
		}

		ReloadHistory[Index].bHasFailed = true;
		break;
	}
}

float ULoadedAmmo::GetReloadRate() const
{
	float ModifiedReloadRate = ReloadRate;
	if (UPlayerClassComponent* PlayerClassComponent = GetOwningFireMode()->GetPlayerClassComponent())
	{
		PlayerClassComponent->OnProcessReloadRate.Broadcast(GetOwningWeapon(), this, ModifiedReloadRate);
	}

	return ModifiedReloadRate;
}

float ULoadedAmmo::GetReloadAmount() const
{
	if (IsIndividualReload())
	{
		return FMath::Min(GetIndividualReloadAmount(), GetAmmoAmount());
	}

	return FMath::Min(GetMaxLoadedAmmoAmount() - GetLoadedAmmoAmount(), GetAmmoAmount());
}

void ULoadedAmmo::OnRep_LoadedAmmoAmount(float PreviousAmount)
{
	OnLoadedAmmoChanged.Broadcast(this, LoadedAmmoAmount);
}

void ULoadedAmmo::OnRep_ReloadCounter()
{
	if (!GetOwningWeapon() || GetOwningWeapon()->IsPuttingDown() || GetOwningWeapon()->IsInactive())
	{
		return;
	}

	if (ReloadCounter > 0)
	{
		Reload();
	}
	else
	{
		StopReload();
	}
}

void ULoadedAmmo::ReloadComplete(float ReloadStartTime)
{
	GetWorld()->GetTimerManager().ClearTimer(ReloadTimer);

	const bool bIsLocallyOwnedRemote = GetOwningWeapon() && GetOwningWeapon()->IsLocallyOwnedRemote();
	bool bIsFailedReload = false;

	for (int32 Index = ReloadHistory.Num() - 1; Index >= 0; Index--)
	{
		if (ReloadStartTime != ReloadHistory[Index].ReloadTimeStamp)
		{
			continue;
		}

		if (ReloadHistory[Index].bHasFailed)
		{
			bIsFailedReload = true;
			break;
		}

		ReloadHistory[Index].bHasApplied = true;
		break;
	}

	if (!bIsFailedReload)
	{
		const float AmmoToRefill = GetReloadAmount();
		AmmoAmount -= AmmoToRefill;
		OnRep_AmmoAmount(AmmoAmount + AmmoToRefill);
		LoadedAmmoAmount += AmmoToRefill;
		OnRep_LoadedAmmoAmount(LoadedAmmoAmount - AmmoToRefill);
		OnReloadComplete.Broadcast(this);
		MARK_PROPERTY_DIRTY_FROM_NAME(UAmmo, AmmoAmount, this);
		MARK_PROPERTY_DIRTY_FROM_NAME(ULoadedAmmo, LoadedAmmoAmount, this);
	}

	if (ShouldAutoRepeatReload() && !GetOwningFireMode()->IsHoldingFire() && !GetOwningFireMode()->IsFiring() && GetOwningWeapon()->IsLocallyOwned() && CanReload())
	{
		Reload();
	}
}