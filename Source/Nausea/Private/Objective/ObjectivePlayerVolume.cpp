// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "Objective/ObjectivePlayerVolume.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "System/CoreGameMode.h"
#include "System/NauseaGameState.h"
#include "Character/CoreCharacter.h"
#include "Gameplay/StatusComponent.h"

UObjectivePlayerVolume::UObjectivePlayerVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UObjectivePlayerVolume::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(UObjectivePlayerVolume, OverlappingCharacters, PushReplicationParams::Default);
}

void UObjectivePlayerVolume::SetObjectiveState(EObjectiveState State)
{
	if (State == GetObjecitveState())
	{
		return;
	}

	Super::SetObjectiveState(State);

	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	switch (State)
	{
	case EObjectiveState::Active:
		BindObjectiveEvents();
		GenerateInitialOverlaps();
		break;
	default:
		UnBindObjectiveEvents();
	}
}

FString UObjectivePlayerVolume::DescribeObjectiveToGameplayDebugger() const
{
	if (GetObjecitveState() == EObjectiveState::Active)
	{
		return FString::Printf(TEXT("%s {white}%i / %i"), *GetName(), GetNumberOfOverlappingCharacters(), GetNumberOfConsideredCharacters());
	}

	return GetName();
}

UObjectivePlayerVolume* UObjectivePlayerVolume::CreatePlayerVolumeObjective(UObject* WorldContextObject, TSubclassOf<UObjectivePlayerVolume> ObjectiveClass, const FString& InObjectiveID, TArray<AActor*> InOverlapActors)
{
	UObjectivePlayerVolume* Objective = CreateObjective<UObjectivePlayerVolume>(WorldContextObject, ObjectiveClass, InObjectiveID);

	if (!Objective)
	{
		return nullptr;
	}

	Objective->OverlapActors = InOverlapActors;
	return Objective;
}

float UObjectivePlayerVolume::GetPercentCharactersInObjective() const
{
	//Special handling.
	switch (GetObjecitveState())
	{
	case EObjectiveState::Completed:
		return 1.f;
	case EObjectiveState::Failed:
	case EObjectiveState::Inactive:
		return 0.f;
	}

	ANauseaGameState* NauseaGameState = Cast<ANauseaGameState>(GetWorld()->GetGameState());

	if (!NauseaGameState)
	{
		return 0.f;
	}

	int32 NumPlayers = GetNumberOfConsideredCharacters();

	if (NumPlayers == 0)
	{
		return 0.f;
	}

	return float(OverlappingCharacters.Num()) / float(NumPlayers);
}

int32 UObjectivePlayerVolume::GetNumberOfConsideredCharacters() const
{
	if (ANauseaGameState* NauseaGameState = Cast<ANauseaGameState>(GetWorld()->GetGameState()))
	{
		return bPercentOfAliveCharacters ? NauseaGameState->GetNumberOfAlivePlayers() : NauseaGameState->GetNumberOfPlayers();
	}

	return 0;
}

int32 UObjectivePlayerVolume::GetNumberOfOverlappingCharacters() const
{
	return OverlappingCharacters.Num();
}

TArray<ACoreCharacter*> UObjectivePlayerVolume::GetOverlappingCharacters() const
{
	TArray<ACoreCharacter*> OutArray;
	OutArray.Reserve(OverlappingCharacters.Num());

	for (const TWeakObjectPtr<ACoreCharacter>& Character : OverlappingCharacters)
	{
		if (!Character.IsValid())
		{
			continue;
		}

		OutArray.Add(Character.Get());
	}

	OutArray.Shrink();

	return OutArray;
}

void UObjectivePlayerVolume::BindObjectiveEvents()
{
	OverlapActors.Remove(nullptr);

	for (AActor* Actor : OverlapActors)
	{
		if (!Actor)
		{
			continue;
		}

		Actor->OnActorBeginOverlap.AddDynamic(this, &UObjectivePlayerVolume::OnOverlapBegin);
		Actor->OnActorEndOverlap.AddDynamic(this, &UObjectivePlayerVolume::OnOverlapEnd);
	}

	if (ACoreGameMode* CoreGameMode = GetWorld()->GetAuthGameMode<ACoreGameMode>())
	{
		CoreGameMode->OnPlayerKilled.AddDynamic(this, &UObjectivePlayerVolume::NotifyKilled);
	}
}

void UObjectivePlayerVolume::UnBindObjectiveEvents()
{
	OverlapActors.Remove(nullptr);

	for (AActor* Actor : OverlapActors)
	{
		if (!Actor)
		{
			continue;
		}

		Actor->OnActorBeginOverlap.RemoveDynamic(this, &UObjectivePlayerVolume::OnOverlapBegin);
		Actor->OnActorEndOverlap.RemoveDynamic(this, &UObjectivePlayerVolume::OnOverlapEnd);
	}

	TArray<TWeakObjectPtr<ACoreCharacter>> PreviousOverlappingCharacters = OverlappingCharacters;
	OverlappingCharacters.Empty();
	OnRep_OverlappingCharacters(PreviousOverlappingCharacters);
	MARK_PROPERTY_DIRTY_FROM_NAME(UObjectivePlayerVolume, OverlappingCharacters, this);
}

void UObjectivePlayerVolume::GenerateInitialOverlaps()
{
	for (AActor* Actor : OverlapActors)
	{
		if (!Actor)
		{
			continue;
		}

		TArray<AActor*> InitialOverlappingActorList;
		Actor->GetOverlappingActors(InitialOverlappingActorList);

		for (AActor* OverlappingActor : InitialOverlappingActorList)
		{
			OnOverlapBegin(Actor, OverlappingActor);
		}
	}
}

void UObjectivePlayerVolume::OnOverlapBegin(AActor* OverlapActor, AActor* OtherActor)
{
	if (!OtherActor)
	{
		return;
	}

	ACoreCharacter* Character = Cast<ACoreCharacter>(OtherActor);

	if (!Character)
	{
		return;
	}

	AddCharacter(Character);
}

void UObjectivePlayerVolume::OnOverlapEnd(AActor* OverlapActor, AActor* OtherActor)
{
	if (!OtherActor)
	{
		return;
	}

	ACoreCharacter* Character = Cast<ACoreCharacter>(OtherActor);

	if (!Character)
	{
		return;
	}

	bool bStillOverlapping = false;

	for (AActor* Actor : OverlapActors)
	{
		if (!Actor)
		{
			continue;
		}

		if (Actor->IsOverlappingActor(Character))
		{
			bStillOverlapping = true;
			break;
		}
	}

	if (!bStillOverlapping)
	{
		RemoveCharacter(Character);
	}
}

void UObjectivePlayerVolume::AddCharacter(ACoreCharacter* Character)
{
	TWeakObjectPtr<ACoreCharacter> CharacterWeakPtr(Character);

	if (!CharacterWeakPtr.IsValid() || CharacterWeakPtr->IsDead() || OverlappingCharacters.Contains(CharacterWeakPtr))
	{
		return;
	}

	CharacterWeakPtr->OnDestroyed.AddDynamic(this, &UObjectivePlayerVolume::CharacterDestroyed);
	
	if (CharacterWeakPtr->GetStatusComponent())
	{
		CharacterWeakPtr->GetStatusComponent()->OnDied.AddDynamic(this, &UObjectivePlayerVolume::CharacterKilled);
	}

	TArray<TWeakObjectPtr<ACoreCharacter>> PreviousOverlappingCharacters = OverlappingCharacters;
	OverlappingCharacters.AddUnique(CharacterWeakPtr);
	OnRep_OverlappingCharacters(PreviousOverlappingCharacters);
	MARK_PROPERTY_DIRTY_FROM_NAME(UObjectivePlayerVolume, OverlappingCharacters, this);
}

void UObjectivePlayerVolume::RemoveCharacter(ACoreCharacter* Character)
{
	TWeakObjectPtr<ACoreCharacter> CharacterWeakPtr(Character);

	if (!CharacterWeakPtr.IsValid() || !OverlappingCharacters.Contains(CharacterWeakPtr))
	{
		return;
	}

	CharacterWeakPtr->OnDestroyed.RemoveDynamic(this, &UObjectivePlayerVolume::CharacterDestroyed);

	if (CharacterWeakPtr->GetStatusComponent())
	{
		CharacterWeakPtr->GetStatusComponent()->OnDied.RemoveDynamic(this, &UObjectivePlayerVolume::CharacterKilled);
	}

	TArray<TWeakObjectPtr<ACoreCharacter>> PreviousOverlappingCharacters = OverlappingCharacters;
	OverlappingCharacters.Remove(CharacterWeakPtr);
	OnRep_OverlappingCharacters(PreviousOverlappingCharacters);
	MARK_PROPERTY_DIRTY_FROM_NAME(UObjectivePlayerVolume, OverlappingCharacters, this);
}

void UObjectivePlayerVolume::CharacterDestroyed(AActor* Character)
{
	RemoveCharacter(Cast<ACoreCharacter>(Character));
}

void UObjectivePlayerVolume::CharacterKilled(UStatusComponent* Component, float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!Component)
	{
		return;
	}

	RemoveCharacter(Cast<ACoreCharacter>(Component->GetOwner()));
}

void UObjectivePlayerVolume::NotifyKilled(AController* Killer, AController* Killed, ACoreCharacter* KilledCharacter, const struct FDamageEvent& DamageEvent)
{
	//Allow UObjectivePlayerVolume::CharacterKilled to handle this as it clearly has not yet.
	if (OverlappingCharacters.Contains(TWeakObjectPtr<ACoreCharacter>(KilledCharacter)))
	{
		return;
	}

	K2_OnPlayerKilled(Killer, Killed, KilledCharacter, DamageEvent);
}

void UObjectivePlayerVolume::OnRep_OverlappingCharacters(TArray<TWeakObjectPtr<ACoreCharacter>> PreviousOverlappingCharacters)
{
	for (TWeakObjectPtr<ACoreCharacter> Character : PreviousOverlappingCharacters)
	{
		if (Character.IsValid() && !OverlappingCharacters.Contains(Character))
		{
			K2_OnPlayerEntered(Character.Get());
			OnPlayerExitedObjective.Broadcast(this, Character.Get());
		}
	}

	for (TWeakObjectPtr<ACoreCharacter> Character : OverlappingCharacters)
	{
		if (Character.IsValid() && !PreviousOverlappingCharacters.Contains(Character))
		{
			K2_OnPlayerExited(Character.Get());
			OnPlayerEnteredObjective.Broadcast(this, Character.Get());
		}
	}

	K2_OnPlayerListChanged();
	OnPlayerListChanged.Broadcast(this);
}