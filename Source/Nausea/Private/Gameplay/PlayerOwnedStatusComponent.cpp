// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Gameplay/PlayerOwnedStatusComponent.h"
#include "Player/PlayerOwnershipInterface.h"
#include "Player/CorePlayerState.h"
#include "Player/PlayerClassComponent.h"

UPlayerOwnedStatusComponent::UPlayerOwnedStatusComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutomaticallyInitialize = false;
	bHideStatusComponentTeam = true;
}

void UPlayerOwnedStatusComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void UPlayerOwnedStatusComponent::InitializeComponent()
{
	//Perform initialization here if not authority (APawn::SetPlayerDefaults will not run on simulated/autonomous proxies).
	//NOTE: bNetStartup means the player owned actor was loaded from level load, meaning it likely might be some time until possession/initialization.
	bAutomaticallyInitialize |= GetOwnerRole() != ROLE_Authority && !GetOwner()->bNetStartup;

	Super::InitializeComponent();
}

void UPlayerOwnedStatusComponent::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	bUseStatusComponentTeam = true;
	Super::SetGenericTeamId(NewTeamID);
}

FGenericTeamId UPlayerOwnedStatusComponent::GetGenericTeamId() const
{
	if (bUseStatusComponentTeam)
	{
		return TeamId;
	}

	if (GetOwningPlayerState())
	{
		return GetOwningPlayerState()->GetGenericTeamId();
	}

	return FGenericTeamId::NoTeam;
}

void UPlayerOwnedStatusComponent::InitializeStatusComponent()
{
	if (GetOwnerInterface())
	{
		return;
	}

	Super::InitializeStatusComponent();

	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	IPlayerOwnershipInterface* PlayerOwnershipInterface = Cast<IPlayerOwnershipInterface>(GetOwner());
	ensure(PlayerOwnershipInterface);
	
	if (!PlayerOwnershipInterface)
	{
		return;
	}

	CorePlayerState = PlayerOwnershipInterface->GetOwningPlayerState<ACorePlayerState>();
	
	if (!GetOwningPlayerState())
	{
		return;
	}

	if (!IsDead())
	{
		GetOwningPlayerState()->SetIsAlive(true);
	}

	if (!GetOwningPlayerState()->OnPlayerClassChanged.IsAlreadyBound(this, &UPlayerOwnedStatusComponent::OnPlayerClassChanged))
	{
		GetOwningPlayerState()->OnPlayerClassChanged.AddDynamic(this, &UPlayerOwnedStatusComponent::OnPlayerClassChanged);
	}

	if (GetOwningPlayerState()->GetPlayerClassComponent())
	{
		OnPlayerClassChanged(GetOwningPlayerState(), GetOwningPlayerState()->GetPlayerClassComponent());
		SetHealth(GetHealthMax());
		SetArmour(GetArmourMax());
	}
}

void UPlayerOwnedStatusComponent::TakeDamage(AActor* Actor, float& DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(Actor, DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void UPlayerOwnedStatusComponent::Died(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (GetOwningPlayerState())
	{
		GetOwningPlayerState()->SetIsAlive(false);

		if (GetOwningPlayerState()->OnPlayerClassChanged.IsAlreadyBound(this, &UPlayerOwnedStatusComponent::OnPlayerClassChanged))
		{
			GetOwningPlayerState()->OnPlayerClassChanged.RemoveDynamic(this, &UPlayerOwnedStatusComponent::OnPlayerClassChanged);
		}
	}

	Super::Died(Damage, DamageEvent, EventInstigator, DamageCauser);
}

void UPlayerOwnedStatusComponent::OnPlayerClassChanged(ACorePlayerState* OwningCorePlayerState, UPlayerClassComponent* PlayerClassComponent)
{
	//Clean up.
	if (DamageTakenDelegate.IsValid())
	{
		OnProcessDamageTaken.Remove(DamageTakenDelegate);
	}

	if (DamageDealtDelegate.IsValid())
	{
		OnProcessDamageDealt.Remove(DamageDealtDelegate);
	}

	if (!PlayerClassComponent)
	{
		SetMaxHealth(Health.DefaultMaxValue);
		SetMaxArmour(Armour.DefaultMaxValue);
		return;
	}

	//Apply player class bonuses.
	float ModifiedMaxHealth = Health.DefaultMaxValue;
	PlayerClassComponent->OnProcessOwnedActorMaxHealth.Broadcast(GetOwner(), ModifiedMaxHealth);
	SetMaxHealth(ModifiedMaxHealth);

	float ModifiedMaxArmour = Armour.DefaultMaxValue;
	PlayerClassComponent->OnProcessOwnedActorMaxHealth.Broadcast(GetOwner(), ModifiedMaxArmour);
	SetMaxArmour(ModifiedMaxArmour);

	TWeakObjectPtr<UPlayerClassComponent> WeakPlayerClassComponent = PlayerClassComponent;
	DamageTakenDelegate = OnProcessDamageTaken.AddWeakLambda(PlayerClassComponent, [WeakPlayerClassComponent](UStatusComponent* StatusComponent, float& DamageAmount, const struct FDamageEvent& DamageEvent, ACorePlayerState* PlayerState)
	{
		if (WeakPlayerClassComponent.IsValid())
		{
			WeakPlayerClassComponent->OnProcessDamageTaken.Broadcast(StatusComponent, DamageAmount, DamageEvent, PlayerState);
		}
	});
	
	DamageDealtDelegate = OnProcessDamageDealt.AddWeakLambda(PlayerClassComponent, [WeakPlayerClassComponent](UStatusComponent* StatusComponent, float& DamageAmount, const struct FDamageEvent& DamageEvent, ACorePlayerState* PlayerState)
	{
		if (WeakPlayerClassComponent.IsValid())
		{
			WeakPlayerClassComponent->OnProcessDamageDealt.Broadcast(StatusComponent, DamageAmount, DamageEvent, PlayerState);
		}
	});
}

ACorePlayerState* UPlayerOwnedStatusComponent::GetOwningPlayerState() const
{
	return CorePlayerState;
}