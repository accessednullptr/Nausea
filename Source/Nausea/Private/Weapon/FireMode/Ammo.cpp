// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Weapon/FireMode/Ammo.h"
#include "Net/UnrealNetwork.h"
#include "Engine/NetDriver.h"
#include "NauseaNetDefines.h"
#include "System/CoreGameplayStatics.h"
#include "Character/CoreCharacter.h"
#include "Player/PlayerClassComponent.h"
#include "Weapon/Weapon.h"
#include "Weapon/FireMode/WeaponFireMode.h"

DEFINE_LOG_CATEGORY(LogAmmo);

UAmmo::UAmmo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetSkipReplicationLogic(ESkipReplicationLogic::SkipOwnerInitial);
}

void UAmmo::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass());
	if (BPClass != NULL)
	{
		BPClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}

	DOREPLIFETIME_WITH_PARAMS_FAST(UAmmo, AmmoAmount, PushReplicationParams::SkipOwner);
	DOREPLIFETIME_WITH_PARAMS_FAST(UAmmo, MaxAmmoAmount, PushReplicationParams::SkipOwner);
	DOREPLIFETIME_WITH_PARAMS_FAST(UAmmo, InitialAmmo, PushReplicationParams::OwnerOnly);
}

void UAmmo::PostInitProperties()
{
	Super::PostInitProperties();
	
	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		return;
	}

	Initialize(GetTypedOuter<UFireMode>());
}

void UAmmo::BeginDestroy()
{
	OwningFireMode = nullptr;
	WorldPrivate = nullptr;

	Super::BeginDestroy();
}

int32 UAmmo::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	check(GetOuter());
	return GetOuter()->GetFunctionCallspace(Function, Stack);
}

bool UAmmo::CallRemoteFunction(UFunction* Function, void* Parameters, FOutParmRec* OutParms, FFrame* Stack)
{
	//RPCs to ammunition is only supported if attached to a firemode.
	if (!GetOwningFireMode())
	{
		return false;
	}

	bool bProcessed = false;

	if (AActor* MyOwner = GetOwningFireMode()->GetOwningCharacter())
	{
		FWorldContext* const Context = GEngine->GetWorldContextFromWorld(GetWorld());
		if (Context != nullptr)
		{
			for (FNamedNetDriver& Driver : Context->ActiveNetDrivers)
			{
				if (Driver.NetDriver != nullptr && Driver.NetDriver->ShouldReplicateFunction(MyOwner, Function))
				{
					Driver.NetDriver->ProcessRemoteFunction(MyOwner, Function, Parameters, OutParms, Stack, this);
					bProcessed = true;
				}
			}
		}
	}
	return bProcessed;
}

void UAmmo::Initialize(UFireMode* FireMode)
{
	if (OwningFireMode)
	{
		OwningFireMode->UnregisterReplicatedSubobject(this);
	}

	//We've been reparented to something that isn't a firemode, attempt to establish some basic initialization.
	if (!FireMode)
	{
		OwningFireMode = nullptr;

		if (AActor* Actor = GetTypedOuter<AActor>())
		{
			WorldPrivate = Actor->GetWorld();
		}
		return;
	}

	OwningFireMode = FireMode;
	OwningFireMode->RegisterReplicatedSubobject(this);
	WorldPrivate = OwningFireMode->GetWorld();

	if (!GetTypedOuter<AActor>() || GetTypedOuter<AActor>()->GetLocalRole() < ROLE_AutonomousProxy)
	{
		bDoneFirstInitialization = true;
		return;
	}

	if (bDoneFirstInitialization)
	{
		return;
	}

	InitialAmmo = DefaultAmmoAmount;
	OnRep_InitialAmount(); //Server and owning client are expected to run the same code path here in order to maintain cohesion.
	MARK_PROPERTY_DIRTY_FROM_NAME(UAmmo, InitialAmmo, this);

	bDoneFirstInitialization = true;
}

const UAmmo* UAmmo::GetDefaultObject() const
{
	if (UWeaponFireMode* WeaponFireMode = Cast<UWeaponFireMode>(OwningFireMode))
	{
		return WeaponFireMode->GetAmmoDefaultObject();
	}

	return nullptr;
}

//Would be nice to do this on UAmmo::Initialize but that runs before the rest of the weapon is setup.
void UAmmo::UpdateAmmoCapacity(bool bFirstInitialization)
{
	float ModifiedAmmoCapacity = GetDefaultObject()->MaxAmmoAmount;
	if (UPlayerClassComponent* PlayerClassComponent = GetOwningFireMode()->GetPlayerClassComponent())
	{
		PlayerClassComponent->OnProcessAmmoCapacity.Broadcast(GetOwningWeapon(), this, ModifiedAmmoCapacity);
	}

	MaxAmmoAmount = FMath::FloorToInt(ModifiedAmmoCapacity);
	MARK_PROPERTY_DIRTY_FROM_NAME(UAmmo, MaxAmmoAmount, this);

	if (!bFirstInitialization)
	{
		return;
	}

	DefaultAmmoAmount *= FMath::FloorToInt(ModifiedAmmoCapacity / GetDefaultObject()->MaxAmmoAmount); //Apply bonus to default ammo amount on first init.
	AmmoAmount = DefaultAmmoAmount;
}

UWeapon* UAmmo::GetOwningWeapon() const
{
	if (!GetOwningFireMode() || !GetOwningFireMode()->GetOwningWeapon())
	{
		return GetTypedOuter<UWeapon>();
	}

	return GetOwningFireMode()->GetOwningWeapon();
}

ACoreCharacter* UAmmo::GetOwningCharacter() const
{
	if (!GetOwningFireMode())
	{
		return GetTypedOuter<ACoreCharacter>();
	}

	return GetOwningFireMode()->GetOwningCharacter();
}

bool UAmmo::ConsumeAmmo(float Amount)
{
	if (!GetOwningFireMode())
	{
		UE_LOG(LogAmmo, VeryVerbose, TEXT("%f: Character %s %s UAmmo::ConsumeAmmo returned false due to %s's OwningFireMode being nullptr."),
			GetWorld()->GetTimeSeconds(), *GetNameSafe(GetOwningCharacter()), *UCoreGameplayStatics::GetNetRoleNameForActor(GetOwningCharacter()), *GetName());
		return false;
	}

	if (GetOwningFireMode()->GetOwningWeapon()->IsSimulatedProxy())
	{
		return true;
	}

	if (!CanConsumeAmmo(Amount))
	{
		UE_LOG(LogAmmo, VeryVerbose, TEXT("%f: Character %s %s UAmmo::ConsumeAmmo returned false due to %s's CanConsumeAmmo returning false."),
			GetWorld()->GetTimeSeconds(), *GetNameSafe(GetOwningCharacter()), *UCoreGameplayStatics::GetNetRoleNameForActor(GetOwningCharacter()), *GetName());
		return false;
	}

	const float PreviousAmount = AmmoAmount;
	AmmoAmount -= Amount;
	OnRep_AmmoAmount(PreviousAmount);
	MARK_PROPERTY_DIRTY_FROM_NAME(UAmmo, AmmoAmount, this);
	return true;
}

bool UAmmo::CanConsumeAmmo(float Amount) const
{
	return Amount <= AmmoAmount;
}

void UAmmo::ApplyAmmoCorrection(float Amount)
{
	const float PreviousAmount = AmmoAmount;
	AmmoAmount += Amount;
	OnRep_AmmoAmount(PreviousAmount);
}

void UAmmo::Client_Reliable_SendAmmoDeltaCorrection_Implementation(float Amount)
{
	ApplyAmmoCorrection(Amount);
}

bool UAmmo::CanReload() const
{
	if (!GetOwningFireMode() || !GetOwningFireMode()->GetOwningWeapon())
	{
		UE_LOG(LogAmmo, VeryVerbose, TEXT("%f: Character %s %s UAmmo::CanReload returned false due to %s's OwningFireMode or OwningWeapon being nullptr."),
			GetWorld()->GetTimeSeconds(), *GetNameSafe(GetOwningCharacter()), *UCoreGameplayStatics::GetNetRoleNameForActor(GetOwningCharacter()), *GetName());
		return false;
	}

	if (GetOwningWeapon()->IsSimulatedProxy())
	{
		return true;
	}

	if (IsReloading())
	{
		UE_LOG(LogAmmo, VeryVerbose, TEXT("%f: Character %s %s UAmmo::CanReload returned false due to UAmmo::IsReloading() returned true."),
			GetWorld()->GetTimeSeconds(), *GetNameSafe(GetOwningCharacter()), *UCoreGameplayStatics::GetNetRoleNameForActor(GetOwningCharacter()), *GetName());
		return false;
	}

	if (!GetOwningFireMode()->GetOwningWeapon()->CanReload(GetOwningFireMode()))
	{
		UE_LOG(LogAmmo, VeryVerbose, TEXT("%f: Character %s %s UAmmo::ConsumeAmmo returned false due to %s's owning weapon CanReload returned false."),
			GetWorld()->GetTimeSeconds(), *GetNameSafe(GetOwningCharacter()), *UCoreGameplayStatics::GetNetRoleNameForActor(GetOwningCharacter()), *GetName());
		return false;
	}

	return true;
}

void UAmmo::ForceReloadComplete()
{
	ReloadComplete(-1.f);
}

bool UAmmo::CanPutDown() const
{
	return !BlockAction();
}

bool UAmmo::BlockAction(const UFireMode* InstigatorFireMode) const
{
	return false;
}

bool UAmmo::HasAnyDescriptors(int32 Flags) const
{
	return (int32(AmmoDescriptor) & Flags) != 0;
}

bool UAmmo::Server_Reliable_Reload_Validate(float WorldTimeOverride)
{
	return true;
}

void UAmmo::Server_Reliable_Reload_Implementation(float WorldTimeOverride)
{
	if(IsReloading() && IsReloadNearlyComplete())
	{
		ReloadComplete(-1.f);
	}

	if (Reload(WorldTimeOverride))
	{
		return;
	}

	UE_LOG(LogAmmo, Verbose, TEXT("%f: Character %s %s UAmmo::Server_Reliable_Reload failed to reload ammo %s."),
		GetWorld()->GetTimeSeconds(), *GetNameSafe(GetOwningCharacter()), *UCoreGameplayStatics::GetNetRoleNameForActor(GetOwningCharacter()), *GetName());

	Client_Reliable_ReloadFailed(WorldTimeOverride);
}

void UAmmo::Client_Reliable_ReloadFailed_Implementation(float WorldTimeOverride)
{

}

void UAmmo::OnRep_AmmoAmount(float PreviousAmount)
{
	OnAmmoChanged.Broadcast(this, AmmoAmount);
}

void UAmmo::OnRep_InitialAmount()
{
	if (UWeapon* Weapon = GetOwningFireMode() ? GetOwningFireMode()->GetOwningWeapon() : nullptr)
	{
		if (Weapon->IsSimulatedProxy())
		{
			return;
		}
	}

	AmmoAmount = InitialAmmo;

	OnRep_AmmoAmount(AmmoAmount);
}

UWorld* UAmmo::GetWorld_Uncached() const
{
	UWorld* ObjectWorld = nullptr;

	const UObject* Owner = GetOwningFireMode();

	if (Owner && !Owner->HasAnyFlags(RF_ClassDefaultObject))
	{
		ObjectWorld = Owner->GetWorld();
	}

	if (ObjectWorld == nullptr)
	{
		if (AActor* Actor = GetTypedOuter<AActor>())
		{
			ObjectWorld = Owner->GetWorld();
		}
		else
		{
			ObjectWorld = Cast<UWorld>(GetOuter());
		}
	}

	return ObjectWorld;
}