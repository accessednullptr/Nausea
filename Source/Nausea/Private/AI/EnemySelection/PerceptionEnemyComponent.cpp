// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "AI/EnemySelection/PerceptionEnemyComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Perception/AISense_Sight.h"
#include "AI/CoreAIController.h"
#include "AI/CoreAIPerceptionComponent.h"
#include "Character/CoreCharacter.h"

UPerceptionEnemyComponent::UPerceptionEnemyComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UPerceptionEnemyComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningPerceptionComponent = Cast<UCoreAIPerceptionComponent>(GetAIController()->GetAIPerceptionComponent());
	
	if (!GetPerceptionComponent()->OnGainedSightOfActor.IsAlreadyBound(this, &UPerceptionEnemyComponent::GainedVisibilityOfActor))
	{
		GetPerceptionComponent()->OnGainedSightOfActor.AddDynamic(this, &UPerceptionEnemyComponent::GainedVisibilityOfActor);
	}

	if (!GetPerceptionComponent()->OnLostSightOfActor.IsAlreadyBound(this, &UPerceptionEnemyComponent::LostVisiblityOfActor))
	{
		GetPerceptionComponent()->OnLostSightOfActor.AddDynamic(this, &UPerceptionEnemyComponent::LostVisiblityOfActor);
	}

	if (!GetPerceptionComponent()->OnHeardNoiseFromActor.IsAlreadyBound(this, &UPerceptionEnemyComponent::HeardNoiseFromActor))
	{
		GetPerceptionComponent()->OnHeardNoiseFromActor.AddDynamic(this, &UPerceptionEnemyComponent::HeardNoiseFromActor);
	}

	if (!GetPerceptionComponent()->OnReceivedDamageFromActor.IsAlreadyBound(this, &UPerceptionEnemyComponent::DamageReceivedFromActor))
	{
		GetPerceptionComponent()->OnReceivedDamageFromActor.AddDynamic(this, &UPerceptionEnemyComponent::DamageReceivedFromActor);
	}

	ensure(GetPerceptionComponent());
}

void UPerceptionEnemyComponent::GetAllEnemies(TArray<AActor*>& EnemyList) const
{
	GetPerceptionComponent()->GetHostileActors(EnemyList);
}

AActor* UPerceptionEnemyComponent::FindBestEnemy() const
{
	if (!GetOwningCharacter() || !GetPerceptionComponent())
	{
		return nullptr;
	}

	float ClosestDistanceSq = MAX_FLT;
	AActor* ClosestActor = nullptr;

	TArray<AActor*> PerceivedHostileActorList;
	GetPerceptionComponent()->GetPerceivedHostileActors(PerceivedHostileActorList);

	for (AActor* HostileActor : PerceivedHostileActorList)
	{
		const bool bCanSee = GetPerceptionComponent()->HasSeenActor(HostileActor);

		const float DistanceSq = bCanSee ? GetOwningCharacter()->GetSquaredDistanceTo(HostileActor) * 0.5f : GetOwningCharacter()->GetSquaredDistanceTo(HostileActor);

		if (DistanceSq >= ClosestDistanceSq)
		{
			continue;
		}

		ClosestDistanceSq = DistanceSq;
		ClosestActor = HostileActor;
	}

	return ClosestActor;
}

void UPerceptionEnemyComponent::GainedVisibilityOfActor(UCoreAIPerceptionComponent* PerceptionComponent, AActor* Actor)
{
	if (Actor == GetEnemy())
	{
		return;
	}

	if (!GetEnemy())
	{
		SetEnemy(Actor);
		return;
	}

	APawn* Pawn = GetAIController()->GetPawn();

	const float DistanceToOldTarget = Pawn->GetDistanceTo(GetEnemy());
	const float DistanceToNewTarget = Pawn->GetDistanceTo(Actor);

	//If we can see our enemy, the newly seen actor must be half the distance of the current enemy.
	if (GetPerceptionComponent()->HasSeenActor(GetEnemy()))
	{
		if (DistanceToNewTarget > DistanceToOldTarget * 0.5f)
		{
			return;
		}
	}
	else
	{
		if (DistanceToNewTarget > DistanceToOldTarget * 2.f)
		{
			return;
		}
	}

	SetEnemy(Actor);
}

void UPerceptionEnemyComponent::LostVisiblityOfActor(UCoreAIPerceptionComponent* PerceptionComponent, AActor* Actor)
{
	if (Actor != GetEnemy())
	{
		return;
	}

	TArray<AActor*> Actors;
	GetPerceptionComponent()->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), Actors);

	float ClosestDistanceSq = MAX_FLT;
	AActor* ClosestActor = nullptr;

	const FVector Location = GetAIController()->GetPawn()->GetActorLocation();

	for (AActor* PerceivedActor : Actors)
	{
		if (!PerceivedActor)
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(Location, PerceivedActor->GetActorLocation());

		if (ClosestDistanceSq > DistanceSq)
		{
			ClosestDistanceSq = DistanceSq;
			ClosestActor = PerceivedActor;
		}
	}

	const float EnemyDistanceSq = FVector::DistSquared(Location, GetEnemy()->GetActorLocation());

	if (EnemyDistanceSq < ClosestDistanceSq)
	{
		return;
	}

	SetEnemy(ClosestActor);
}

void UPerceptionEnemyComponent::HeardNoiseFromActor(UCoreAIPerceptionComponent* PerceptionComponent, AActor* Actor)
{
	if (!GetPerceptionComponent()->HasSeenActor(Actor))
	{
		return;
	}

	APawn* Pawn = GetAIController()->GetPawn();

	const float DistanceToOldTarget = Pawn->GetDistanceTo(GetEnemy());
	const float DistanceToNewTarget = Pawn->GetDistanceTo(Actor);

	if (DistanceToNewTarget > DistanceToOldTarget * 0.5f)
	{
		return;
	}

	SetEnemy(Actor);
}

void UPerceptionEnemyComponent::DamageReceivedFromActor(UCoreAIPerceptionComponent* PerceptionComponent, AActor* Actor, float DamageThreat)
{
	if (!GetPerceptionComponent()->HasSeenActor(Actor))
	{
		return;
	}

	APawn* Pawn = GetAIController()->GetPawn();

	const float DistanceToOldTarget = Pawn->GetDistanceTo(GetEnemy());
	const float DistanceToNewTarget = Pawn->GetDistanceTo(Actor);

	if (DistanceToNewTarget > DistanceToOldTarget * 1.2f)
	{
		return;
	}

	SetEnemy(Actor);
}