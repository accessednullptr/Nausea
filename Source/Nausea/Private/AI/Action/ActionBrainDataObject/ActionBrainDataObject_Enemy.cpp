// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#include "AI/Action/ActionBrainDataObject/ActionBrainDataObject_Enemy.h"
#include "AI/CoreAIController.h"
#include "AI/EnemySelectionComponent.h"

UActionBrainDataObject_Enemy::UActionBrainDataObject_Enemy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UActionBrainDataObject_Enemy::Initialize(AAIController* InOwningController)
{
	Super::Initialize(InOwningController);

	if (ACoreAIController* CoreAIController = Cast<ACoreAIController>(OwningController))
	{
		OwningEnemySelectionComponent = CoreAIController->GetEnemySelectionComponent();
	}
}

void UActionBrainDataObject_Enemy::GetListOfActors(TArray<AActor*>& ActorList) const
{
	switch (SelectionMode)
	{
	case EDataSelectionMode::LocationOnly:
		return;
	case EDataSelectionMode::PreferLocation:
		TArray<FVector> LocationList;
		GetListOfLocations(LocationList);
		if (LocationList.Num() > 0) { return; }
	}

	ActorList.Empty(FMath::Max(1, ActorList.Max()));
	if (!OwningEnemySelectionComponent || !OwningEnemySelectionComponent->GetEnemy())
	{
		return;
	}

	ActorList.Add(OwningEnemySelectionComponent->GetEnemy());
}

void UActionBrainDataObject_Enemy::GetListOfLocations(TArray<FVector>& LocationList) const
{
	switch (SelectionMode)
	{
	case EDataSelectionMode::ActorOnly:
		return;
	case EDataSelectionMode::PreferActor:
		TArray<AActor*> ActorList;
		GetListOfActors(ActorList);
		if (ActorList.Num() > 0) { return; }
	}

	LocationList.Empty(FMath::Max(1, LocationList.Max()));
	if (!OwningEnemySelectionComponent || !OwningEnemySelectionComponent->GetEnemy())
	{
		return;
	}

	LocationList.Add(OwningEnemySelectionComponent->GetEnemy()->GetActorLocation());
}

UActionBrainDataObject_AllEnemies::UActionBrainDataObject_AllEnemies(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UActionBrainDataObject_AllEnemies::Initialize(AAIController* InOwningController)
{
	Super::Initialize(InOwningController);
}

void UActionBrainDataObject_AllEnemies::GetListOfActors(TArray<AActor*>& ActorList) const
{
	switch (SelectionMode)
	{
	case EDataSelectionMode::LocationOnly:
		return;
	case EDataSelectionMode::PreferLocation:
		TArray<FVector> LocationList;
		GetListOfLocations(LocationList);
		if (LocationList.Num() > 0) { return; }
	}

	ActorList.Empty(FMath::Max(1, ActorList.Max()));
	if (!OwningEnemySelectionComponent || !OwningEnemySelectionComponent->GetEnemy())
	{
		return;
	}

	OwningEnemySelectionComponent->GetAllEnemies(ActorList);
}

void UActionBrainDataObject_AllEnemies::GetListOfLocations(TArray<FVector>& LocationList) const
{
	switch (SelectionMode)
	{
	case EDataSelectionMode::ActorOnly:
		return;
	case EDataSelectionMode::PreferActor:
		TArray<AActor*> ActorList;
		GetListOfActors(ActorList);
		if (ActorList.Num() > 0) { return; }
	}

	LocationList.Empty(FMath::Max(1, LocationList.Max()));
	if (!OwningEnemySelectionComponent || !OwningEnemySelectionComponent->GetEnemy())
	{
		return;
	}

	TArray<AActor*> ActorList;
	ActorList.Empty(LocationList.Num());

	OwningEnemySelectionComponent->GetAllEnemies(ActorList);

	for (AActor* Actor : ActorList)
	{
		LocationList.Add(Actor->GetActorLocation());
	}
}