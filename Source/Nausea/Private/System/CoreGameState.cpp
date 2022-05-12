// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "System/CoreGameState.h"
#include "GenericTeamAgentInterface.h"
#include "Nausea.h"
#include "NauseaHelpers.h"
#include "System/NetHelper.h"
#include "System/CoreGameMode.h"
#include "System/SpawnCharacterSystem.h"
#include "Player/CorePlayerState.h"
#include "Gameplay/StatusInterface.h"
#include "Gameplay/StatusComponent.h"
#include "Gameplay/CoreDamageType.h"
#include "Gameplay/DamageLogModifier/DamageLogModifierObject.h"

ACoreGameState::ACoreGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SpawnCharacterSystem = CreateDefaultSubobject<USpawnCharacterSystem>(TEXT("SpawnCharacterSystem"));
}

void ACoreGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(ACoreGameState, PlayerClassList, PushReplicationParams::Default);
	DOREPLIFETIME_WITH_PARAMS_FAST(ACoreGameState, GameDifficulty, PushReplicationParams::Default);
}

void ACoreGameState::BeginPlay()
{
	Super::BeginPlay();

	if (GetLocalRole() != ROLE_Authority)
	{
		for (TSoftClassPtr<UPlayerClassComponent> SoftPlayerClass : DefaultPlayerClassList)
		{
			ensure(SoftPlayerClass.LoadSynchronous());
		}
	}
}

void ACoreGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

	ACorePlayerState* CorePlayerState = Cast<ACorePlayerState>(PlayerState);

	//Remote can't use this event because by this point the local player has not processed the initial bunch yet (this is called from AActor::PostInitializeComponents).
	if (!CorePlayerState || CorePlayerState->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	OnPlayerStateReady(CorePlayerState);
}

void ACoreGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);
	
	ACorePlayerState* CorePlayerState = Cast<ACorePlayerState>(PlayerState);

	if (!CorePlayerState || CorePlayerState->IsInactive())
	{
		return;
	}

	OnPlayerStateRemoved.Broadcast(CorePlayerState->IsActivePlayer(), CorePlayerState);

	UnbindToRelevantPlayerState(CorePlayerState);
}

void ACoreGameState::OnRep_MatchState()
{
	Super::OnRep_MatchState();

	OnMatchStateChanged.Broadcast(this, GetMatchState());
}

void ACoreGameState::InitializeGameState(ACoreGameMode* CoreGameMode)
{
	GameDifficulty = UGameplayStatics::GetIntOption(CoreGameMode->OptionsString, ACoreGameMode::OptionDifficulty, CoreGameMode->DefaultDifficulty);
}

void ACoreGameState::OnPlayerStateReady(ACorePlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return;
	}

	OnPlayerStateAdded.Broadcast(PlayerState->IsActivePlayer(), PlayerState);

	if (!PlayerState->IsActivePlayer())
	{
		UnbindToRelevantPlayerState(PlayerState);
	}
	else
	{
		BindToRelevantPlayerState(PlayerState);
	}
}

bool ACoreGameState::IsWaitingToStart() const
{
	return GetMatchState() == MatchState::WaitingToStart;
}

bool ACoreGameState::IsInProgress() const
{
	return GetMatchState() == MatchState::InProgress;
}

bool ACoreGameState::IsWaitingPostMatch() const
{
	return GetMatchState() == MatchState::WaitingPostMatch;
}

bool ACoreGameState::CanPlayerSelectPlayerClass(const ACorePlayerController* PlayerController, TSubclassOf<UPlayerClassComponent> PlayerClass) const
{
	if (!PlayerClass || !PlayerController)
	{
		return false;
	}

	return true;
}

void ACoreGameState::HandleDamageApplied(AActor* Actor, float& DamageAmount, FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (!Actor || !DamageCauser)
	{
		return;
	}

	ApplyFriendlyFireDamageMultiplier(Actor, DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void ACoreGameState::HandleStatusApplied(AActor* Actor, float& DamageAmount, FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (!Actor || !DamageCauser)
	{
		return;
	}

	ApplyFriendlyFireEffectPowerMultiplier(Actor, DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

bool ACoreGameState::AreThereAnyAlivePlayers() const
{
	for (APlayerState* PlayerState : PlayerArray)
	{
		ACorePlayerState* CorePlayerState = Cast<ACorePlayerState>(PlayerState);

		if (!CorePlayerState || !CorePlayerState->IsAlive())
		{
			continue;
		}

		return true;
	}

	return false;
}

int32 ACoreGameState::GetNumberOfPlayers() const
{
	return PlayerArray.Num();
}

int32 ACoreGameState::GetNumberOfAlivePlayers() const
{
	int32 Count = 0;

	for (APlayerState* PlayerState : PlayerArray)
	{
		ACorePlayerState* CorePlayerState = Cast<ACorePlayerState>(PlayerState);

		if (!CorePlayerState || !CorePlayerState->IsAlive())
		{
			continue;
		}

		Count++;
	}

	return Count;
}

void ACoreGameState::ApplyFriendlyFireDamageMultiplier(AActor* Actor, float& DamageAmount, FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	const UCoreDamageType* CoreDamageType = DamageEvent.DamageTypeClass ? Cast<UCoreDamageType>(DamageEvent.DamageTypeClass.GetDefaultObject()) : nullptr;

	if (!CoreDamageType || CoreDamageType->bCausedByWorld)
	{
		return;
	}

	EApplicationResult Result = CoreDamageType->GetDamageApplicationResult(DamageCauser, Actor);

	float DamageModifier = 1.f;
	switch (Result)
	{
	case EApplicationResult::FriendlyFire:
		DamageAmount *= FriendlyFireDamageMultiplier;
		DamageModifier = FriendlyFireDamageMultiplier;
		break;
	case EApplicationResult::None:
		DamageAmount = 0.f;
		DamageModifier = 0.f;
		break;
	}

	if (DamageModifier != 1.f)
	{
		TScriptInterface<IStatusInterface> StatusInterface = Actor;
		if (UStatusComponent* StatusComponent = TSCRIPTINTERFACE_CALL_FUNC_RET(StatusInterface, GetStatusComponent, K2_GetStatusComponent, nullptr))
		{
			UObject* ResolvedInstigator = nullptr;

			if (IPlayerOwnershipInterface* DamageCauserOwnershipInterface = Cast<IPlayerOwnershipInterface>(DamageCauser))
			{
				if (ACorePlayerState* PlayerState = DamageCauserOwnershipInterface->GetOwningPlayerState())
				{
					ResolvedInstigator = PlayerState;
				}
				else
				{
					ResolvedInstigator = DamageCauserOwnershipInterface->GetOwningPawn();
				}
			}

			if (!ResolvedInstigator)
			{
				ResolvedInstigator = EventInstigator ? EventInstigator->GetPlayerState<ACorePlayerState>() : nullptr;
			}

			StatusComponent->PushDamageLogModifier(FDamageLogEventModifier(UFriendlyFireScalingDamageLogModifier::StaticClass(), ResolvedInstigator, DamageModifier));
		}
	}
}

void ACoreGameState::ApplyFriendlyFireEffectPowerMultiplier(AActor* Actor, float& EffectPower, FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	const UCoreDamageType* CoreDamageType = DamageEvent.DamageTypeClass ? Cast<UCoreDamageType>(DamageEvent.DamageTypeClass.GetDefaultObject()) : nullptr;

	if (!CoreDamageType || CoreDamageType->bCausedByWorld)
	{
		return;
	}

	EApplicationResult Result = CoreDamageType->GetStatusApplicationResult(DamageCauser, Actor);

	switch (Result)
	{
	case EApplicationResult::FriendlyFire:
		EffectPower *= FriendlyFireEffectPowerMultiplier;
		break;
	case EApplicationResult::None:
		EffectPower = 0.f;
		break;
	}
}

void ACoreGameState::OnRep_PlayerClassList()
{

}

void ACoreGameState::BindToRelevantPlayerState(ACorePlayerState* PlayerState)
{
	RelevantCorePlayerStateList.AddUnique(PlayerState);

	if (PlayerState->OnAliveChanged.IsAlreadyBound(this, &ACoreGameState::OnPlayerAliveUpdate))
	{
		return;
	}

	OnPlayerStateBecomeRelevant.Broadcast(PlayerState);
	PlayerState->OnSpectatorChanged.AddDynamic(this, &ACoreGameState::OnPlayerSpectatorUpdate);
	PlayerState->OnAliveChanged.AddDynamic(this, &ACoreGameState::OnPlayerAliveUpdate);
}

void ACoreGameState::UnbindToRelevantPlayerState(ACorePlayerState* PlayerState)
{
	RelevantCorePlayerStateList.Remove(PlayerState);

	if (!PlayerState->OnAliveChanged.IsAlreadyBound(this, &ACoreGameState::OnPlayerAliveUpdate))
	{
		return;
	}

	OnPlayerStateBecomeNonRelevant.Broadcast(PlayerState);
	PlayerState->OnSpectatorChanged.RemoveDynamic(this, &ACoreGameState::OnPlayerSpectatorUpdate);
	PlayerState->OnAliveChanged.RemoveDynamic(this, &ACoreGameState::OnPlayerAliveUpdate);
}

void ACoreGameState::OnPlayerSpectatorUpdate(ACorePlayerState* PlayerState, bool bIsSpectator)
{
	OnPlayerSpectatorChanged.Broadcast(PlayerState, bIsSpectator);
}

void ACoreGameState::OnPlayerAliveUpdate(ACorePlayerState* PlayerState, bool bAlive)
{
	OnPlayerAliveChanged.Broadcast(PlayerState, bAlive);
}

ACoreGameState* ACoreGameState::GetCoreGameState(const UObject* WorldContextObject)
{
	UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	return World ? World->GetGameState<ACoreGameState>() : nullptr;
}