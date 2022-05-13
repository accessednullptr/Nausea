// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Player/PlayerOwnershipInterface.h"
#include "Player/PlayerOwnershipInterfaceTypes.h"
#include "Player/CorePlayerController.h"
#include "Player/CorePlayerState.h"
#include "Character/CoreCharacter.h"

UPlayerOwnershipInterface::UPlayerOwnershipInterface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

AController* IPlayerOwnershipInterface::GetOwningController() const
{
	return GetOwningPlayerState() ? Cast<AController>(GetOwningPlayerState()->GetOwner()) : nullptr;
}

APawn* IPlayerOwnershipInterface::GetOwningPawn() const
{
	if (APlayerState* PlayerState = GetOwningPlayerState())
	{
		return PlayerState->GetPawn();
	}
	else if (AController* Controller = GetOwningController())
	{
		return Controller->GetPawn();
	}
	
	return nullptr;
}

FGenericTeamId IPlayerOwnershipInterface::GetOwningTeamId() const
{
	//ACorePlayerState overrides this function.
	if (!ensure(GetOwningPlayerState() != this))
	{
		return FGenericTeamId::NoTeam;
	}

	if (IPlayerOwnershipInterface* OwningPlayerState = GetOwningPlayerState<IPlayerOwnershipInterface>())
	{
		return OwningPlayerState->GetOwningTeamId();
	}

	return FGenericTeamId::NoTeam;
}

ETeam IPlayerOwnershipInterface::GetOwningTeam() const
{
	return UPlayerOwnershipInterfaceTypes::GetTeamEnumFromGenericTeamId(GetOwningTeamId());
}

UPlayerClassComponent* IPlayerOwnershipInterface::GetPlayerClassComponent() const
{
	if (ACorePlayerState* PlayerState = GetOwningPlayerState<ACorePlayerState>())
	{
		return PlayerState->GetPlayerClassComponent();
	}

	return nullptr;
}

UPlayerStatisticsComponent* IPlayerOwnershipInterface::GetPlayerStatisticsComponent() const
{
	if (ACorePlayerController* PlayerController = GetOwningController<ACorePlayerController>())
	{
		return PlayerController->GetPlayerStatisticsComponent();
	}

	return nullptr;
}

UVoiceComponent* IPlayerOwnershipInterface::GetVoiceComponent() const
{
	if (ACoreCharacter* Character = GetOwningPawn<ACoreCharacter>())
	{
		return Character->GetVoiceComponent();
	}

	return nullptr;
}

UVoiceCommandComponent* IPlayerOwnershipInterface::GetVoiceCommandComponent() const
{
	if (GetOwningPlayerState())
	{
		return GetOwningPlayerState()->GetVoiceCommandComponent();
	}

	return nullptr;
}

UAbilityComponent* IPlayerOwnershipInterface::GetAbilityComponent() const
{
	if (ACoreCharacter* Character = GetOwningPawn<ACoreCharacter>())
	{
		return Character->GetAbilityComponent();
	}

	return nullptr;
}

bool IPlayerOwnershipInterface::IsLocallyOwned() const
{
	if (AController* Controller = GetOwningController())
	{
		return Controller->IsLocalController();
	}

	return false;
}

UPlayerOwnershipSystemLibrary::UPlayerOwnershipSystemLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

ACorePlayerState* UPlayerOwnershipSystemLibrary::GetActorPlayerState(TScriptInterface<IPlayerOwnershipInterface> Target)
{
	if (!Target)
	{
		return nullptr;
	}

	return Target->GetOwningPlayerState();
}

bool UPlayerOwnershipSystemLibrary::IsLocallyOwnedActor(TScriptInterface<IPlayerOwnershipInterface> Target)
{
	if (!Target)
	{
		return false;
	}

	return Target->IsLocallyOwned();
}

ETeam UPlayerOwnershipSystemLibrary::GetActorTeam(TScriptInterface<IPlayerOwnershipInterface> Target)
{
	if (!Target)
	{
		return ETeam::NoTeam;
	}

	return Target->GetOwningTeam();
}

UPlayerClassComponent* UPlayerOwnershipSystemLibrary::GetActorPlayerClass(TScriptInterface<IPlayerOwnershipInterface> Target)
{
	if (!Target)
	{
		return nullptr;
	}

	return Target->GetPlayerClassComponent();
}

UPlayerStatisticsComponent* UPlayerOwnershipSystemLibrary::GetActorPlayerStatistics(TScriptInterface<IPlayerOwnershipInterface> Target)
{
	if (!Target)
	{
		return nullptr;
	}

	return Target->GetPlayerStatisticsComponent();
}

UVoiceCommandComponent* UPlayerOwnershipSystemLibrary::GetActorVoiceCommandComponent(TScriptInterface<IPlayerOwnershipInterface> Target)
{
	if (!Target)
	{
		return nullptr;
	}

	return Target->GetVoiceCommandComponent();
}