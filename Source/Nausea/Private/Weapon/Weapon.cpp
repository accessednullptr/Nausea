// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#include "Weapon/Weapon.h"
#include "Internationalization/StringTableRegistry.h"
#include "Engine/ActorChannel.h"
#include "Animation/AnimInstance.h"
#include "Nausea.h"
#include "System/NetHelper.h"
#include "Character/CoreCharacter.h"
#include "Character/CoreCharacterAnimInstanceTypes.h"
#include "Player/PlayerClassComponent.h"
#include "Weapon/InventoryManagerComponent.h"
#include "Weapon/FireMode.h"
#include "Weapon/FireMode/WeaponFireMode.h"
#include "Weapon/FireMode/Ammo.h"

DECLARE_STATS_GROUP(TEXT("Weapon"), STATGROUP_Weapon, STATCAT_Advanced);

DECLARE_CYCLE_STAT(TEXT("Replicate Subobjects"), STAT_WeaponReplicateSubobjects, STATGROUP_Weapon);

#define LOG_WEAPON_EVENT(Verbosity, EventText, ...)\
UE_LOG(LogWeapon, Verbosity, TEXT(EventText), GetWorld()->GetTimeSeconds(), *GetNameSafe(GetOwningCharacter()), *GetName(), ##__VA_ARGS__);\

UWeapon::UWeapon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetSkipReplicationLogic(ESkipReplicationLogic::SkipOwnerInitialOnNonNetOwner);
	FireModeList.SetNum(MAXFIREMODES);
}

void UWeapon::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(UWeapon, WeaponState, PushReplicationParams::SkipOwner);
}

void UWeapon::Serialize(FArchive& Ar)
{
	if (!Ar.IsSaving() || !HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		Super::Serialize(Ar);
		return;
	}

	if (GetNameSafe(GetClass()).StartsWith("REINST"))
	{
		Super::Serialize(Ar);
		return;
	}

	auto InitializeFireModeData = [this](UFireMode* FireMode, EFireMode Slot)
	{
		if (!FireMode)
		{
			return;
		}

		FireMode->InitializeFireModeSlot(Slot);
		FireMode->RegisterOwningWeaponClass(this);
	};

	InitializeFireModeData(PrimaryFire, EFireMode::Primary);
	InitializeFireModeData(SecondaryFire, EFireMode::Secondary);
	InitializeFireModeData(TertiaryFire, EFireMode::Tertiary);
	InitializeFireModeData(QuaternaryFire, EFireMode::Quaternary);
	InitializeFireModeData(QuinaryFire, EFireMode::Quinary);

	Super::Serialize(Ar);
}

void UWeapon::BeginPlay()
{
	InitializeFireModeList();

	Super::BeginPlay();
}

void UWeapon::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	for (UFireMode* FireMode : FireModeList)
	{
		if (!FireMode)
		{
			continue;
		}

		UWeaponFireMode* WeaponFireMode = Cast<UWeaponFireMode>(FireMode);

		if (!WeaponFireMode)
		{
			FireMode->MarkPendingKill();
			continue;
		}
		
		if (UAmmo* Ammo = WeaponFireMode->GetAmmo())
		{
			Ammo->MarkPendingKill();
		}

		WeaponFireMode->MarkPendingKill();
	}
}

bool UWeapon::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	SCOPE_CYCLE_COUNTER(STAT_WeaponReplicateSubobjects);

	const bool bIsRelevantWeapon = RepFlags && (RepFlags->bNetInitial || bReplicateFireModesWhenInactive || IsCurrentlyEquippedWeapon());

	if (!bIsRelevantWeapon)
	{
		if (HasPendingReplicatedSubobjects(Channel))
		{
			const bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
			return ReplicateSubobjectList(Channel, Bunch, RepFlags) || bWroteSomething;
		}

		return false;
	}
	
	return ReplicateSubobjectList(Channel, Bunch, RepFlags);
}

bool UWeapon::IsCurrentlyEquippedWeapon() const
{
	return GetOwningInventoryManager() ? GetOwningInventoryManager()->GetCurrentWeapon() == this : false;
}

bool UWeapon::IsFiring() const
{
	for (const UFireMode* FireMode : FireModeList)
	{
		if (!FireMode)
		{
			continue;
		}

		if (!FireMode->IsFiring())
		{
			continue;
		}

		return true;
	}

	return false;
}

float UWeapon::GetEquipDuration() const
{
	float ModifierEquipTimeTime = EquipTime;
	if (UPlayerClassComponent* PlayerClassComponent = GetPlayerClassComponent())
	{
		PlayerClassComponent->OnProcessPutDownTime.Broadcast(this, ModifierEquipTimeTime);
	}
	return ModifierEquipTimeTime;
}

float UWeapon::GetPutDownDuration() const
{
	float ModifierPutDownTime = PutDownTime;
	if (UPlayerClassComponent* PlayerClassComponent = GetPlayerClassComponent())
	{
		PlayerClassComponent->OnProcessPutDownTime.Broadcast(this, ModifierPutDownTime);
	}
	return ModifierPutDownTime;
}

UFireMode* UWeapon::GetFireMode(EFireMode Mode) const
{
	switch (Mode)
	{
	case EFireMode::Primary:
		return PrimaryFire;
	}

	return FireModeList.IsValidIndex(uint8(Mode)) ? FireModeList[uint8(Mode)] : nullptr;
}

UAmmo* UWeapon::GetAmmo(EFireMode Mode) const
{
	if (UWeaponFireMode* FireMode = Cast<UWeaponFireMode>(GetFireMode(Mode)))
	{
		return FireMode ? FireMode->GetAmmo() : nullptr;
	}

	return nullptr;
}

const UFireMode* UWeapon::GetFireModeDefaultObject(EFireMode Mode) const
{
	const UWeapon* DefaultThis = GetClass()->GetDefaultObject<UWeapon>();

	if (!DefaultThis)
	{
		return nullptr;
	}

	return DefaultThis->GetFireMode(Mode);
}

const UAmmo* UWeapon::GetAmmoDefaultObject(EFireMode Mode) const
{
	const UWeaponFireMode* DefaultWeaponFireMode = Cast<UWeaponFireMode>(GetFireModeDefaultObject(Mode));

	if (!DefaultWeaponFireMode)
	{
		return nullptr;
	}

	return DefaultWeaponFireMode->GetAmmo();
}

EFireMode UWeapon::GetFireModeEnum(UFireMode* FireMode) const
{
	if (!FireMode)
	{
		return EFireMode::MAX;
	}
	
	const int32 Index = FireModeList.Find(FireMode);
	return Index != INDEX_NONE ? static_cast<EFireMode>(Index) : EFireMode::MAX;
}

EFireMode UWeapon::GetAmmoFireModeEnum(UAmmo* Ammo) const
{
	if (!Ammo || !Ammo->GetOwningFireMode())
	{
		return EFireMode::MAX;
	}

	return GetFireModeEnum(Ammo->GetOwningFireMode());
}

bool UWeapon::Equip()
{
	if (!CanEquip())
	{
		LOG_WEAPON_EVENT(Verbose, "%f: Character %s | Weapon %s UWeapon::Equip failed. UWeapon::CanEquip returned false.");
		return false;
	}

	LOG_WEAPON_EVENT(Verbose, "%f: Character %s | Weapon %s UWeapon::Equip performing equip.");

	ClearPendingPutDown();

	GetWorld()->GetTimerManager().SetTimer(EquipTimer, FTimerDelegate::CreateUObject(this, &UWeapon::EquipComplete), GetEquipDuration(), false);
	K2_OnEquip();
	SetWeaponState(EWeaponState::Equipping);

	if (IsAuthority() && bMakeNoiseOnEquip)
	{
		EquipNoise.MakeNoise(GetOwner());
	}
	
	if (!IsNetMode(NM_DedicatedServer))
	{
		OnEquipCosmetic();
	}

	return true;
}

bool UWeapon::CanEquip() const
{
	if (IsNonOwningAuthority() || IsSimulatedProxy())
	{
		return true;
	}

	if (IsEquipping())
	{
		LOG_WEAPON_EVENT(VeryVerbose, "%f: Character %s | Weapon %s UWeapon::CanEquip returning false. UWeapon::IsEquipping returned true.");
		return false;
	}

	return true;
}

void UWeapon::EquipComplete()
{
	LOG_WEAPON_EVENT(Verbose, "%f: Character %s | Weapon %s UWeapon::EquipComplete equip completed.");

	GetWorld()->GetTimerManager().ClearTimer(EquipTimer);
	K2_OnEquipComplete();
	SetWeaponState(EWeaponState::Active);
}

void UWeapon::OnEquipCosmetic()
{
	ACoreCharacter* Character = GetOwningCharacter();
	
	if (!Character)
	{
		return;
	}

	//Wait one frame before unhiding the weapon mesh.
	auto UnhideWeaponMesh = [](TWeakObjectPtr<USkeletalMeshComponent> SkeletalMesh){
		if (SkeletalMesh.IsValid())
		{
			SkeletalMesh->SetHiddenInGame(false);
		}
	};

	if (USkeletalMeshComponent* SkeletalMesh = GetWeaponMesh1P())
	{
		SkeletalMesh->SetAnimInstanceClass(nullptr);
		SkeletalMesh->SetSkeletalMesh(GetSkeletalMesh1P());

		//No longer uses GripSocket since that's mutated to find the attach transform relative to the hand_r socket.
		SkeletalMesh->AttachToComponent(SkeletalMesh->GetAttachParent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("hand_r"));
		SkeletalMesh->SetRelativeTransform(FirstPersonMeshAttachTransform);

		if (GetWeaponAnimInstance1P())
		{
			SkeletalMesh->SetAnimInstanceClass(GetWeaponAnimInstance1P());
		}

		SkeletalMesh->SetHiddenInGame(true);
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, UnhideWeaponMesh, TWeakObjectPtr<USkeletalMeshComponent>(SkeletalMesh)));
	}

	if (USkeletalMeshComponent* SkeletalMesh = GetWeaponMesh3P())
	{
		SkeletalMesh->SetAnimInstanceClass(nullptr);
		SkeletalMesh->SetSkeletalMesh(GetSkeletalMesh3P());

		SkeletalMesh->AttachToComponent(SkeletalMesh->GetAttachParent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("hand_r"));
		SkeletalMesh->SetRelativeTransform(ThirdPersonMeshAttachTransform);

		if (GetWeaponAnimInstance3P())
		{
			SkeletalMesh->SetAnimInstanceClass(GetWeaponAnimInstance3P());
		}

		SkeletalMesh->SetHiddenInGame(true);
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, UnhideWeaponMesh, TWeakObjectPtr<USkeletalMeshComponent>(SkeletalMesh)));
	}

	Character->UpdateAnimObject(FirstPersonAnimation.GetDefaultObject(), ThirdPersonAnimation.GetDefaultObject());

	if (const UAnimationObject* FirstPersonAnimationObject = Character->GetFirstPersonAnimObject())
	{
		FirstPersonAnimationObject->PlayEquipMontage(Character->GetMesh1P(), Character->GetWeaponMesh1P(), GetEquipDuration());
	}

	if (const UAnimationObject* ThirdPersonAnimationObject = Character->GetFirstPersonAnimObject())
	{
		ThirdPersonAnimationObject->PlayEquipMontage(Character->GetMesh(), Character->GetWeaponMesh(), GetEquipDuration());
	}
}

bool UWeapon::PutDown()
{
	if (!CanPutDown())
	{
		LOG_WEAPON_EVENT(Verbose, "%f: Character %s | Weapon %s UWeapon::PutDown failed. UWeapon::CanPutDown returned false.");
		if (IsLocallyOwned())
		{
			LOG_WEAPON_EVENT(VeryVerbose, "%f: Character %s | Weapon %s UWeapon::PutDown has set bPendingPutDown to true.");
			bPendingPutDown = true;
		}

		return false;
	}

	LOG_WEAPON_EVENT(Verbose, "%f: Character %s | Weapon %s UWeapon::PutDown performing put down.");

	if (IsLocallyOwned())
	{
		StopAllFire();
	}

	ClearPendingPutDown();

	GetWorld()->GetTimerManager().SetTimer(PutDownTimer, FTimerDelegate::CreateUObject(this, &UWeapon::PutDownComplete), GetPutDownDuration(), false);
	K2_OnPutDown();
	SetWeaponState(EWeaponState::PuttingDown);

	if (IsAuthority() && bMakeNoiseOnPutDown)
	{
		PutDownNoise.MakeNoise(GetOwner());
	}

	if (!IsNetMode(NM_DedicatedServer))
	{
		OnPutDownCosmetic();
	}

	return true;
}

bool UWeapon::CanPutDown() const
{
	if (IsNonOwningAuthority() || IsSimulatedProxy())
	{
		return true;
	}

	if (!IsActiveWeapon())
	{
		LOG_WEAPON_EVENT(VeryVerbose, "%f: Character %s | Weapon %s UWeapon::CanEquip returning false. UWeapon::IsActiveWeapon returned true.");
		return false;
	}

	for (UFireMode* FireMode : FireModeList)
	{
		if (!FireMode)
		{
			continue;
		}

		if (!FireMode->CanPutDown())
		{
			LOG_WEAPON_EVENT(VeryVerbose, "%f: Character %s | Weapon %s UWeapon::CanEquip returning false. Firemode %s UFireMode::CanPutDown returned false.", *GetNameSafe(FireMode));
			return false;
		}
	}

	return true;
}

void UWeapon::ClearPendingPutDown()
{
	bPendingPutDown = false;
}

void UWeapon::PutDownComplete()
{
	LOG_WEAPON_EVENT(Verbose, "%f: Character %s | Weapon %s UWeapon::PutDownComplete put down completed.");

	GetWorld()->GetTimerManager().ClearTimer(PutDownTimer);
	K2_OnPutDownComplete();

	SetWeaponState(EWeaponState::Inactive);

	if (!IsNetMode(NM_DedicatedServer))
	{
		OnPutDownCompleteCosmetic();
	}
}

void UWeapon::OnPutDownCosmetic()
{
	ACoreCharacter* Character = GetOwningCharacter();

	if (!Character)
	{
		return;
	}

	if (Character->GetFirstPersonAnimObject())
	{
		Character->GetFirstPersonAnimObject()->PlayPutDownMontage(Character->GetMesh1P(), Character->GetWeaponMesh1P(), GetPutDownDuration());
	}

	if (Character->GetThirdPersonAnimObject())
	{
		Character->GetThirdPersonAnimObject()->PlayPutDownMontage(Character->GetMesh(), Character->GetWeaponMesh(), GetPutDownDuration());
	}
}

void UWeapon::OnPutDownCompleteCosmetic()
{

}

void UWeapon::AbortPutDown()
{
	LOG_WEAPON_EVENT(Verbose, "%f: Character %s | Weapon %s UWeapon::AbortPutDown attempting aborting put down.");
	if (!IsNonOwningAuthority())
	{
		LOG_WEAPON_EVENT(Error, "%f: Character %s | Weapon %s UWeapon::AbortPutDown called on remote.");
		return;
	}

	if (!IsPuttingDown())
	{
		LOG_WEAPON_EVENT(Error, "%f: Character %s | Weapon %s UWeapon::AbortPutDown called when weapon is already in PutDown.");
		return;
	}

	PutDownComplete();
}

void UWeapon::AbortEquip()
{
	LOG_WEAPON_EVENT(Verbose, "%f: Character %s | Weapon %s UWeapon::AbortEquip attempting aborting equip.");
	if (!IsNonOwningAuthority())
	{
		LOG_WEAPON_EVENT(Error, "%f: Character %s | Weapon %s UWeapon::AbortPutDown called on remote.");
		return;
	}

	if (!IsEquipping())
	{
		LOG_WEAPON_EVENT(Error, "%f: Character %s | Weapon %s UWeapon::AbortPutDown called when weapon is already in Equipping.");
		return;
	}

	EquipComplete();
	PutDown();
	PutDownComplete();
}

void UWeapon::ForcePutDown()
{
	LOG_WEAPON_EVENT(Verbose, "%f: Character %s | Weapon %s UWeapon::ForcePutDown attempting forced put down.");
	if (!IsNonOwningAuthority() || IsInactive())
	{
		LOG_WEAPON_EVENT(Error, "%f: Character %s | Weapon %s UWeapon::AbortPutDown called on remote or inactive weapon.");
		return;
	}

	if (IsPuttingDown())
	{
		PutDownComplete();
		return;
	}

	if (IsEquipping())
	{
		AbortEquip();
		return;
	}

	for (UFireMode* FireMode : FireModeList)
	{
		if (!FireMode || FireMode->CanPutDown())
		{
			continue;
		}

		FireMode->ForceEndFire();
	}

	ensure(CanPutDown());

	PutDown();
	PutDownComplete();
}

void UWeapon::ForceEquip()
{
	LOG_WEAPON_EVENT(Verbose, "%f: Character %s | Weapon %s UWeapon::ForceEquip attempting forced equip.");
	if (!IsNonOwningAuthority() || IsActiveWeapon())
	{
		LOG_WEAPON_EVENT(Error, "%f: Character %s | Weapon %s UWeapon::AbortPutDown called on remote or active weapon.");
		return;
	}

	if (IsEquipping())
	{
		LOG_WEAPON_EVENT(VeryVerbose, "%f: Character %s | Weapon %s UWeapon::ForceEquip calling UWeapon::EquipComplete immediately, as weapon is already equipping.");
		EquipComplete();
		return;
	}

	if (IsPuttingDown())
	{
		AbortPutDown();
		return;
	}

	ensure(CanEquip());

	Equip();
	EquipComplete();
}

bool UWeapon::ShouldPlayEffects1P() const
{
	return GetOwningCharacter()->IsCurrentlyFirstPersonViewedPawn();
}

bool UWeapon::ShouldPlayEffects3P() const
{
	return !ShouldPlayEffects1P();
}

USkeletalMeshComponent* UWeapon::GetMesh1P() const
{
	return GetOwningCharacter()->GetMesh1P();
}

USkeletalMeshComponent* UWeapon::GetWeaponMesh1P() const
{
	return GetOwningCharacter()->GetWeaponMesh1P();
}

USkeletalMeshComponent* UWeapon::GetMesh3P() const
{
	return GetOwningCharacter()->GetMesh();
}

USkeletalMeshComponent* UWeapon::GetWeaponMesh3P() const
{
	return GetOwningCharacter()->GetWeaponMesh();
}

bool UWeapon::Fire(EFireMode Mode)
{
	LOG_WEAPON_EVENT(Verbose, "%f: Character %s | Weapon %s UWeapon::Fire attempting fire.");
	if (UFireMode* FireMode = GetFireMode(Mode))
	{
		return FireMode->Fire();
	}

	return false;
}

bool UWeapon::StopFire(EFireMode Mode)
{
	LOG_WEAPON_EVENT(Verbose, "%f: Character %s | Weapon %s UWeapon::Fire attempting stop fire.");
	if (UFireMode* FireMode = GetFireMode(Mode))
	{
		FireMode->StopFire();
		return true;
	}

	return false;
}

void UWeapon::StopAllFire()
{
	LOG_WEAPON_EVENT(Verbose, "%f: Character %s | Weapon %s UWeapon::StopAllFire attempting to stop all fire.");
	for (UFireMode* FireMode : FireModeList)
	{
		if (!FireMode)
		{
			continue;
		}

		//Since firing and holding fire may be distinct concepts for a given weapon (automatics, charged weapons), clear them separately.
		if (FireMode->IsFiring())
		{
			FireMode->StopFire();
		}

		if (FireMode->IsHoldingFire())
		{
			FireMode->ClearHoldingFire();
		}
	}
}

bool UWeapon::Reload(EFireMode Mode)
{
	if (UFireMode* FireMode = GetFireMode(Mode))
	{
		LOG_WEAPON_EVENT(Verbose, "%f: Character %s | Weapon %s UWeapon::Reload attempting to reload %s firemode %s.", *GetFireModeName(Mode).ToString(), *GetNameSafe(FireMode));

		if (FireMode->Reload())
		{
			StopAllFire();
			return true;
		}

		return false;
	}

	return false;
}

bool UWeapon::StopReload(EFireMode Mode)
{
	if (UFireMode* FireMode = GetFireMode(Mode))
	{
		LOG_WEAPON_EVENT(Verbose, "%f: Character %s | Weapon %s UWeapon::StopReload attempting to stop reload of %s firemode %s.", *GetFireModeName(Mode).ToString(), *GetNameSafe(FireMode));

		return FireMode->StopReload();
	}

	return false;
}

void UWeapon::StopAllReload()
{
	LOG_WEAPON_EVENT(Verbose, "%f: Character %s | Weapon %s UWeapon::StopAllReload attempting to stop all reloads.");
	for (UFireMode* FireMode : FireModeList)
	{
		if (!FireMode)
		{
			continue;
		}

		FireMode->StopReload();
	}
}

bool UWeapon::IsActionBlocked(const UFireMode* InstigatorFireMode) const
{
	for (UFireMode* FireMode : FireModeList)
	{
		if (!FireMode)
		{
			continue;
		}

		if (FireMode->BlockAction(InstigatorFireMode))
		{
			LOG_WEAPON_EVENT(VeryVerbose, "%f: Character %s | Weapon %s UWeapon::IsActionBlocked returned true. Firemode %s is blocking action.", *GetNameSafe(FireMode));
			return true;
		}
	}

	return false;
}

bool UWeapon::CanFire(const UFireMode* InstigatorFireMode) const
{
	if (!IsActiveWeapon())
	{
		return false;
	}

	if (IsActionBlocked(InstigatorFireMode))
	{
		return false;
	}
	
	return true;
}

bool UWeapon::CanReload(UFireMode* InstigatorFireMode) const
{
	if (!IsActiveWeapon())
	{
		return false;
	}

	if (IsActionBlocked(InstigatorFireMode))
	{
		return false;
	}

	return true;
}

void UWeapon::SetWeaponState(EWeaponState State)
{
	if (GetOwnerRole() < ROLE_AutonomousProxy)
	{
		return;
	}

	EWeaponState PreviousState = WeaponState;

	WeaponState = State;

	OnRep_WeaponState(PreviousState);
}

void UWeapon::OnRep_WeaponState(EWeaponState PreviousState)
{
	if (IsSimulatedProxy())
	{
		switch (WeaponState)
		{
		case EWeaponState::Equipping:
			GetOwningInventoryManager()->SetCurrentWeapon(this); //Has special handler for simulated proxies.
			break;
		case EWeaponState::PuttingDown:
			PutDown();
			break;
		case EWeaponState::Active:
			GetOwningInventoryManager()->SetCurrentWeapon(this);
			if (PreviousState == EWeaponState::Equipping) { EquipComplete(); }
			break;
		case EWeaponState::Inactive:
			if (PreviousState == EWeaponState::PuttingDown) { PutDownComplete(); }
			break;
		}
	}

	OnWeaponStateChanged.Broadcast(this, WeaponState, PreviousState);

	switch (WeaponState)
	{
	case EWeaponState::Equipping:
		GetOwningInventoryManager()->RequestMovementSpeedModifierUpdate();
		OnWeaponEquipStart.Broadcast(this);
		break;
	case EWeaponState::PuttingDown:
		OnWeaponPutDownStart.Broadcast(this);
		break;
	case EWeaponState::Active:
		if (PreviousState == EWeaponState::Equipping) { OnWeaponEquipComplete.Broadcast(this); }
		break;
	case EWeaponState::Inactive:
		GetOwningInventoryManager()->RequestMovementSpeedModifierUpdate();
		if (PreviousState == EWeaponState::PuttingDown) { OnWeaponPutDownComplete.Broadcast(this); }
		break;
	}
}

void UWeapon::InitializeFireModeList()
{
	auto InitializeFireModeData = [this](UFireMode* FireMode)
	{
		if (!FireMode)
		{
			return;
		}

		RegisterFireMode(FireMode);

		if (IsAuthority() && FireMode->IsReplicated())
		{
			RegisterReplicatedSubobject(FireMode);
		}
	};

	InitializeFireModeData(PrimaryFire);
	InitializeFireModeData(SecondaryFire);
	InitializeFireModeData(TertiaryFire);
	InitializeFireModeData(QuaternaryFire);
	InitializeFireModeData(QuinaryFire);
}

void UWeapon::FireCompleted(UFireMode* FireMode)
{
	OnWeaponFireComplete.Broadcast(this, FireMode);
	CheckPendingPutDown();
}

void UWeapon::ReloadCompleted(UAmmo* Ammo)
{
	OnWeaponReloadComplete.Broadcast(this, Ammo);

	//Only the local client can check queued requests.
	if (!IsLocallyOwned())
	{
		return;
	}

	//Pending put down has priority over other checks.
	CheckPendingPutDown();

	if (!IsActiveWeapon())
	{
		return;
	}

	for (UFireMode* FireMode : FireModeList)
	{
		if (FireMode && FireMode->IsHoldingFire())
		{
			FireMode->Fire();
			return;
		}
	}
}

void UWeapon::RegisterFireMode(UFireMode* FireMode)
{
	const EFireMode Slot = FireMode->GetFireMode();

	if (!ensure(Slot != EFireMode::MAX))
	{
		return;
	}

	if (FireModeList[int32(Slot)])
	{
		ensure(FireModeList[int32(Slot)] == FireMode);
		return;
	}

	FireModeList[int32(Slot)] = FireMode;
}

bool UWeapon::HasAnyDescriptors(int32 Flags) const
{
	return (int32(WeaponDescriptor) & Flags) != 0;
}

void UWeapon::CheckPendingPutDown()
{
	if (!IsLocallyOwned())
	{
		return;
	}

	if (!IsPendingPutDown() || !GetOwningInventoryManager() || !GetOwningInventoryManager()->GetPendingWeapon())
	{
		return;
	}

	if (!CanPutDown())
	{
		return;
	}

	OnWeaponAwaitingPutDown.Broadcast(this);
}

FText UWeapon::GetWeaponNameFromClass(TSubclassOf<UWeapon> WeaponClass)
{
	const UWeapon* WeaponClassCDO = WeaponClass.GetDefaultObject();
	return WeaponClassCDO ? WeaponClassCDO->GetWeaponName() : FText();
}

FText UWeapon::GetWeaponDescriptionFromClass(TSubclassOf<UWeapon> WeaponClass)
{
	const UWeapon* WeaponClassCDO = WeaponClass.GetDefaultObject();
	return WeaponClassCDO ? WeaponClassCDO->GetWeaponName() : FText();
}

TSoftObjectPtr<UTexture> UWeapon::GetInventoryItemImageFromClass(TSubclassOf<UWeapon> WeaponClass)
{
	const UWeapon* WeaponClassCDO = WeaponClass.GetDefaultObject();
	return WeaponClassCDO ? WeaponClassCDO->GetInventoryItemImage() : nullptr;
}

EWeaponGroup UWeapon::GetWeaponGroupFromClass(TSubclassOf<UWeapon> WeaponClass)
{
	const UWeapon* WeaponClassCDO = WeaponClass.GetDefaultObject();
	return WeaponClassCDO ? WeaponClassCDO->GetWeaponGroup() : EWeaponGroup::None;
}

static FText WeaponStateActive = LOCTABLE("/Game/Localization/WeaponStringTable.WeaponStringTable", "WeaponState_Active");
static FText WeaponStateInactive = LOCTABLE("/Game/Localization/WeaponStringTable.WeaponStringTable", "WeaponState_Inactive");
static FText WeaponStateEquipping = LOCTABLE("/Game/Localization/WeaponStringTable.WeaponStringTable", "WeaponState_Equipping");
static FText WeaponStatePuttingDown = LOCTABLE("/Game/Localization/WeaponStringTable.WeaponStringTable", "WeaponState_PuttingDown");
static FText WeaponStateCustom = LOCTABLE("/Game/Localization/WeaponStringTable.WeaponStringTable", "WeaponState_Custom");
static FText WeaponStateNone = LOCTABLE("/Game/Localization/WeaponStringTable.WeaponStringTable", "WeaponState_None");
FText UWeapon::GetWeaponStateName(EWeaponState State)
{
	switch (State)
	{
	case EWeaponState::Active:
		return WeaponStateActive;
	case EWeaponState::Inactive:
		return WeaponStateInactive;
	case EWeaponState::Equipping:
		return WeaponStateEquipping;
	case EWeaponState::PuttingDown:
		return WeaponStatePuttingDown;
	case EWeaponState::Custom:
		return WeaponStateCustom;
	case EWeaponState::None:
		return WeaponStateNone;
	}

	return WeaponStateNone;
}

static FText FireModePrimary = LOCTABLE("/Game/Localization/WeaponStringTable.WeaponStringTable", "FireMode_Primary");
static FText FireModeSecondary = LOCTABLE("/Game/Localization/WeaponStringTable.WeaponStringTable", "FireMode_Secondary");
static FText FireModeTertiary = LOCTABLE("/Game/Localization/WeaponStringTable.WeaponStringTable", "FireMode_Tertiary");
static FText FireModeQuaternary = LOCTABLE("/Game/Localization/WeaponStringTable.WeaponStringTable", "FireMode_Quaternary");
static FText FireModeQuinary = LOCTABLE("/Game/Localization/WeaponStringTable.WeaponStringTable", "FireMode_Quinary");
static FText FireModeMAX = LOCTABLE("/Game/Localization/WeaponStringTable.WeaponStringTable", "FireMode_Invalid");
FText UWeapon::GetFireModeName(EFireMode FireMode)
{
	switch (FireMode)
	{
	case EFireMode::Primary:
		return FireModePrimary;
	case EFireMode::Secondary:
		return FireModeSecondary;
	case EFireMode::Tertiary:
		return FireModeTertiary;
	case EFireMode::Quaternary:
		return FireModeQuaternary;
	case EFireMode::Quinary:
		return FireModeQuinary;
	case EFireMode::MAX:
		return FireModeMAX;
	}

	return FireModeMAX;
}