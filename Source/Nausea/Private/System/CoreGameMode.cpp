// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#include "System/CoreGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "System/CoreGameState.h"
#include "Player/CorePlayerState.h"
#include "Character/CoreCharacter.h"
#include "Player/PlayerOwnershipInterface.h"
#include "Gameplay/StatusComponent.h"

const FString ACoreGameMode::OptionDifficulty = FString(TEXT("Difficulty"));
const FString ACoreGameMode::OptionModifiers = FString(TEXT("Modifiers"));

ACoreGameMode::ACoreGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameStateClass = ACoreGameState::StaticClass();
}

void ACoreGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	UGameplayStatics::GetIntOption(Options, OptionDifficulty, DefaultDifficulty);
}

void ACoreGameMode::InitGameState()
{
	Super::InitGameState();

	check(GetGameState<ACoreGameState>());
	GetGameState<ACoreGameState>()->InitializeGameState(this);
}

void ACoreGameMode::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	Super::FinishRestartPlayer(NewPlayer, StartRotation);

	if (UStatusComponent* StatusComponent = NewPlayer ? UStatusInterfaceStatics::GetStatusComponent(NewPlayer->GetPawn()) : nullptr)
	{
		StatusComponent->OnDied.AddDynamic(this, &ACoreGameMode::PlayerKilled);
	}
}

void ACoreGameMode::Broadcast(AActor* Sender, const FString& Msg, FName Type)
{
	IPlayerOwnershipInterface* PlayerOwnershipInterface = Cast<IPlayerOwnershipInterface>(Sender);

	if (!PlayerOwnershipInterface)
	{
		Super::Broadcast(Sender, Msg, Type);
		return;
	}

	APlayerState* SenderPlayerState = PlayerOwnershipInterface->GetOwningPlayerState();

	if (!SenderPlayerState)
	{
		Super::Broadcast(Sender, Msg, Type);
		return;
	}

	const FString ResizedString = Msg.Len() > 128 ? Msg.Left(128) : Msg;

	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		if (APlayerController* PC = Iterator->Get())
		{
			PC->ClientTeamMessage(SenderPlayerState, ResizedString, Type);
		}
	}
}

void ACoreGameMode::Say(const FString& Msg)
{
	ensureMsgf(false, TEXT("ACoreGameMode::Say called. Please directly call ACoreGameMode::Broadcast instead so that Sender and Type context is provided."));
}

bool ACoreGameMode::ReadyToStartMatch_Implementation()
{
	if (!Super::ReadyToStartMatch_Implementation())
	{
		return false;
	}

	bool bHasReadyPlayer = false;
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		if (PlayerController && PlayerController->CanRestartPlayer())
		{
			bHasReadyPlayer = true;
			break;
		}
	}

	if (!bHasReadyPlayer)
	{
		return false;
	}

	return true;
}

uint8 ACoreGameMode::GetGameDifficulty() const
{
	if (const ACoreGameState* CoreGameState = GetGameState<ACoreGameState>())
	{
		return CoreGameState->GetGameDifficulty();
	}

	return GetDefaultGameDifficulty();
}

void ACoreGameMode::PlayerKilled(UStatusComponent* Component, float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	ACoreCharacter* Character = Component ? Cast<ACoreCharacter>(Component->GetOwner()) : nullptr;

	if (!Character)
	{
		return;
	}

	NotifyKilled(EventInstigator, Character->GetController(), Character, DamageEvent);
}

void ACoreGameMode::NotifyKilled(AController* Killer, AController* Killed, ACoreCharacter* KilledCharacter, const struct FDamageEvent& DamageEvent)
{
	OnPlayerKilled.Broadcast(Killer, Killed, KilledCharacter, DamageEvent);
}