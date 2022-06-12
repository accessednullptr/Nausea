// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "AI/EnemySelection/AggroEnemyComponent.h"
#include "Player/PlayerOwnershipInterface.h"
#include "Player/CorePlayerState.h"
#include "Character/CoreCharacter.h"
#include "AI/CoreAIPerceptionComponent.h"

FAggroData::FAggroData(AActor* InActor, float InThreat)
	: Actor(InActor), Threat(InThreat)
{

}

bool FAggroData::TickAggroEntry(float DeltaTime, const bool bIsPerceived)
{
	Threat = bIsPerceived ? FMath::Max(Threat - (DeltaTime * (bIsPerceived ? 1.f : 3.f)), 1.f) : Threat - (DeltaTime * (bIsPerceived ? 1.f : 3.f));
	return Threat <= 0.f;
}

UAggroEnemyComponent::UAggroEnemyComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UAggroEnemyComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UCoreAIPerceptionComponent* PerceptionComponent = GetPerceptionComponent();

	TArray<TObjectKey<AActor>> RemovalKeyList;
	RemovalKeyList.Reserve(AggroDataMap.Num());

	SortedAggroKeyList.Empty(AggroDataMap.Num());

	auto ThreatSorter = [this](const TObjectKey<AActor>& A, const TObjectKey<AActor>& B)
	{
		return this->AggroDataMap[A] < this->AggroDataMap[B];
	};

	for (TPair<TObjectKey<AActor>, FAggroData>& Entry : AggroDataMap)
	{
		if (!Entry.Key.ResolveObjectPtr())
		{
			RemovalKeyList.Add(Entry.Key);
			continue;
		}

		if (Entry.Value.TickAggroEntry(DeltaTime, PerceptionComponent->HasPerceivedActor(Entry.Key.ResolveObjectPtr(), -1.f)))
		{
			RemovalKeyList.Add(Entry.Key);
			continue;
		}

		SortedAggroKeyList.HeapPush(Entry.Key, ThreatSorter);
	}
	SortedAggroKeyList.Shrink();

	for (const TObjectKey<AActor>& Key : RemovalKeyList)
	{
		AggroDataMap.Remove(Key);
	}

	if (SetEnemy(FindBestEnemy()) && AggroDataMap.Num() == 0)
	{
		SetComponentTickEnabled(false);
		return;
	}
}

void UAggroEnemyComponent::OnPawnUpdated(ACoreAIController* AIController, ACoreCharacter* InCharacter)
{
	if (!InCharacter)
	{
		CleanupAggroData();
	}

	Super::OnPawnUpdated(AIController, InCharacter);
}

void UAggroEnemyComponent::GetAllEnemies(TArray<AActor*>& EnemyList) const
{
	auto ThreatSorter = [this](const AActor& A, const AActor& B)
	{
		return this->AggroDataMap[&A] < this->AggroDataMap[&B];
	};

	EnemyList.Reserve(AggroDataMap.Num());
	for (const TPair<TObjectKey<AActor>, FAggroData>& Entry : AggroDataMap)
	{
		AActor* Actor = Entry.Key.ResolveObjectPtr();

		if (!Actor)
		{
			continue;
		}

		EnemyList.HeapPush(Actor, ThreatSorter);
	}
}

AActor* UAggroEnemyComponent::FindBestEnemy() const
{
	for (const TObjectKey<AActor>& Key : SortedAggroKeyList)
	{
		AActor* Actor = Key.ResolveObjectPtr();
		TScriptInterface<IAITargetInterface> AITargetInterface = TScriptInterface<IAITargetInterface>(Actor);
		if (TSCRIPTINTERFACE_CALL_FUNC_RET(AITargetInterface, IsTargetable, K2_IsTargetable, false, GetOwningCharacter()))
		{
			return Actor;
		}
	}

	return nullptr;
}

void UAggroEnemyComponent::GainedVisibilityOfActor(UCoreAIPerceptionComponent* PerceptionComponent, AActor* Actor)
{
	UpdateActorAggro(Actor);
}

void UAggroEnemyComponent::LostVisiblityOfActor(UCoreAIPerceptionComponent* PerceptionComponent, AActor* Actor)
{

}

void UAggroEnemyComponent::HeardNoiseFromActor(UCoreAIPerceptionComponent* PerceptionComponent, AActor* Actor)
{
	UpdateActorAggro(Actor);
}

void UAggroEnemyComponent::DamageReceivedFromActor(UCoreAIPerceptionComponent* PerceptionComponent, AActor* Actor, float DamageThreat)
{
	UpdateActorAggro(Actor, DamageThreat);
}

void UAggroEnemyComponent::CleanupAggroData()
{
	AggroDataMap.Empty();
	SortedAggroKeyList.Empty();
}

void UAggroEnemyComponent::UpdateActorAggro(AActor* Actor, float Threat)
{
	if (!AggroDataMap.Contains(Actor))
	{
		AggroDataMap.Emplace(Actor, MoveTempIfPossible(FAggroData(Actor, Threat)));
	}
	else
	{
		AggroDataMap[Actor].AddThreat(Threat);
	}

	SetComponentTickEnabled(true);
}