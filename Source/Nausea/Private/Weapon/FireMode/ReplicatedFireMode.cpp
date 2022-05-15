// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Weapon/FireMode/ReplicatedFireMode.h"
#include "Engine/NetDriver.h"
#include "GameFramework/GameStateBase.h"
#include "System/NetHelper.h"
#include "Character/CoreCharacter.h"
#include "Weapon/Weapon.h"

UReplicatedFireMode::UReplicatedFireMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UReplicatedFireMode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass());
	if (BPClass != NULL)
	{
		BPClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}

	DOREPLIFETIME_WITH_PARAMS_FAST(UReplicatedFireMode, FireCounter, PushReplicationParams::SkipOwner);
}

int32 UReplicatedFireMode::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	check(GetOuter());
	return GetOuter()->GetFunctionCallspace(Function, Stack);
}

bool UReplicatedFireMode::CallRemoteFunction(UFunction* Function, void* Parameters, FOutParmRec* OutParms, FFrame* Stack)
{
	bool bProcessed = false;

	if (AActor* MyOwner = GetOwningCharacter())
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

bool UReplicatedFireMode::Fire(float WorldTimeOverride)
{
	if (!Super::Fire(WorldTimeOverride))
	{
		return false;
	}

	if (GetOwningWeapon()->IsLocallyOwnedRemote() && GetWorld()->GetGameState())
	{
		SendFireRequest();
	}

	return true;
}

void UReplicatedFireMode::StopFire(float WorldTimeOverride)
{
	Super::StopFire(WorldTimeOverride);

	if (GetOwningWeapon()->IsLocallyOwnedRemote() && GetWorld()->GetGameState())
	{
		SendStopFireRequest();
	}
}

void UReplicatedFireMode::SendFireRequest()
{
	Server_Reliable_Fire(GetWorld()->GetGameState()->GetServerWorldTimeSeconds());
}

void UReplicatedFireMode::SendStopFireRequest()
{
	Server_Reliable_StopFire(GetWorld()->GetGameState()->GetServerWorldTimeSeconds());
}

bool UReplicatedFireMode::Server_Reliable_Fire_Validate(float WorldTimeOverride)
{
	return true;
}

void UReplicatedFireMode::Server_Reliable_Fire_Implementation(float WorldTimeOverride)
{
	if (!GetWorld()->GetGameState())
	{
		return;
	}

	if (!GetOwningWeapon()->IsInactive() && !GetOwningWeapon()->IsActiveWeapon())
	{
		UE_LOG(LogFireMode, Error, TEXT("%f: Character %s UReplicatedFireMode::Server_Reliable_Fire tried to fire while %s was in state %s."),
			GetWorld()->GetTimeSeconds(),
			*GetNameSafe(GetOwningCharacter()),
			*GetNameSafe(GetOwningWeapon()),
			*UWeapon::GetWeaponStateName(GetOwningWeapon()->GetWeaponState()).ToString());
	
		GetOwningWeapon()->ForceEquip();
	}

	WorldTimeOverride = FMath::Min(WorldTimeOverride, GetWorld()->GetGameState()->GetServerWorldTimeSeconds());

	if (!Fire(WorldTimeOverride))
	{
		Client_Reliable_FailedFire();
	}

	FireCounter++;
	MARK_PROPERTY_DIRTY_FROM_NAME(UReplicatedFireMode, FireCounter, this);
}

bool UReplicatedFireMode::Server_Reliable_StopFire_Validate(float WorldTimeOverride)
{
	return true;
}

void UReplicatedFireMode::Server_Reliable_StopFire_Implementation(float WorldTimeOverride)
{
	if (!GetWorld()->GetGameState())
	{
		return;
	}

	WorldTimeOverride = FMath::Min(WorldTimeOverride, GetWorld()->GetGameState()->GetServerWorldTimeSeconds());
	StopFire(WorldTimeOverride);
}

void UReplicatedFireMode::Client_Reliable_FailedFire_Implementation()
{
	
}

void UReplicatedFireMode::OnRep_FireCounter()
{
	if (!GetOwningWeapon() || !GetOwningWeapon()->HasBegunPlay())
	{
		LocalFireCounter = FireCounter;
		return;
	}

	if (FireCounter <= 0)
	{
		LocalFireCounter = FireCounter;
		return;
	}

	if (GetOwningWeapon()->IsInactive())
	{
		LocalFireCounter = FireCounter;
		return;
	}

	UpdateFireCounter();
}