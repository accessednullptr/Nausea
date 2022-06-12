// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Character/CoreCharacterComponent.h"
#include "Player/CorePlayerState.h"
#include "Character/CoreCharacter.h"
#include "Gameplay/StatusComponent.h"

UCoreCharacterComponent::UCoreCharacterComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UCoreCharacterComponent::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		return;
	}

	//Some things want to know what this is very early so we have to handle that.
	OwningCharacter = GetTypedOuter<ACoreCharacter>();
}

void UCoreCharacterComponent::BeginPlay()
{
	UpdateOwningCharacter();

	Super::BeginPlay();
}

void UCoreCharacterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	OnComponentEndPlay.Broadcast(this, EndPlayReason);
}

ACorePlayerState* UCoreCharacterComponent::GetOwningPlayerState() const
{
	//Fallback for authorities that might be performing this initialization very early.
	if (!GetOwningPawn()->GetPlayerState() && GetOwningController())
	{
		return GetOwningController()->GetPlayerState<ACorePlayerState>();
	}

	return GetOwningPawn()->GetPlayerState<ACorePlayerState>();
}

AController* UCoreCharacterComponent::GetOwningController() const
{
	return GetOwningPawn()->GetController();
}

APawn* UCoreCharacterComponent::GetOwningPawn() const
{
	if (!OwningCharacter)
	{
		return GetTypedOuter<APawn>();
	}

	return OwningCharacter;
}

bool UCoreCharacterComponent::IsLocallyOwnedRemote() const
{
	return GetOwnerRole() == ROLE_AutonomousProxy;
}

bool UCoreCharacterComponent::IsSimulatedProxy() const
{
	return GetOwnerRole() < ROLE_AutonomousProxy;
}

bool UCoreCharacterComponent::IsAuthority() const
{
	return GetOwnerRole() == ROLE_Authority;
}

bool UCoreCharacterComponent::IsNonOwningAuthority() const
{
	return IsAuthority() && !IsLocallyOwned();
}

ACoreCharacter* UCoreCharacterComponent::GetOwningCharacter() const
{
	return OwningCharacter;
}

APilotCharacter* UCoreCharacterComponent::GetOwningPilotCharacter() const
{
	return OwningCharacter;
}

bool UCoreCharacterComponent::IsLocallyOwned() const
{
	return IsLocallyOwnedRemote() || (GetOwningCharacter() ? GetOwningCharacter()->IsLocallyControlled() : false);
}

void UCoreCharacterComponent::UpdateOwningCharacter()
{
	if (OwningCharacter)
	{
		return;
	}

	OwningCharacter = GetTypedOuter<ACoreCharacter>();

	if (bBindToCharacterDied)
	{
		if (ACoreCharacter* CoreCharacter = GetOwningCharacter())
		{
			if (UStatusComponent* StatusComponent = CoreCharacter->GetStatusComponent())
			{
				StatusComponent->OnDeathEvent.AddDynamic(this, &UCoreCharacterComponent::OnOwningCharacterDied);

				if (CoreCharacter->IsDead())
				{
					const FDeathEvent& DeathEvent = StatusComponent->GetDeathEvent();
					OnOwningCharacterDied(StatusComponent, DeathEvent.DamageType, DeathEvent.Damage, DeathEvent.HitLocation, DeathEvent.HitMomentum);
				}
			}
		}
	}
}