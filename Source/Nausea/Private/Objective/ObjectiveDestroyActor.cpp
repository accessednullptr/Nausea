// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Objective/ObjectiveDestroyActor.h"
#include "GameFramework/Actor.h"
#include "Gameplay/StatusInterface.h"
#include "Gameplay/StatusComponent.h"

UObjectiveDestroyActor::UObjectiveDestroyActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UObjectiveDestroyActor::SetObjectiveState(EObjectiveState State)
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
		break;
	default:
		UnBindObjectiveEvents();
	}
}

FString UObjectiveDestroyActor::DescribeObjectiveToGameplayDebugger() const
{
	if (GetObjecitveState() == EObjectiveState::Active)
	{
		return FString::Printf(TEXT("%s {white}%i / %i"), *GetName(), RemainingActorList.Num(), ActorList.Num());
	}

	return GetName();
}

UObjectiveDestroyActor* UObjectiveDestroyActor::CreateDestroyActorObjective(UObject* WorldContextObject, TSubclassOf<UObjectiveDestroyActor> ObjectiveClass, const FString& InObjectiveID, TArray<AActor*> InActors)
{
	UObjectiveDestroyActor* Objective = CreateObjective<UObjectiveDestroyActor>(WorldContextObject, ObjectiveClass, InObjectiveID);

	if (!Objective)
	{
		return nullptr;
	}

	Objective->ActorList.Reserve(InActors.Num());
	for (AActor* Actor : InActors)
	{
		Objective->ActorList.Add(Actor);
	}

	return Objective;
}

void UObjectiveDestroyActor::BindObjectiveEvents()
{
	ActorList.Remove(nullptr);
	RemainingActorList = ActorList;

	for (TWeakObjectPtr<AActor> Actor : ActorList)
	{
		if (!Actor.IsValid())
		{
			continue;
		}

		if (UStatusComponent* StatusComponent = UStatusInterfaceStatics::GetStatusComponent(Actor.Get()))
		{
			StatusComponent->OnDied.AddDynamic(this, &UObjectiveDestroyActor::OnActorDied);
		}

		Actor->OnDestroyed.AddDynamic(this, &UObjectiveDestroyActor::CharacterDestroyed);
	}

	UpdateObjectiveProgress();
}

void UObjectiveDestroyActor::UnBindObjectiveEvents()
{
	ActorList.Remove(nullptr);

	for (TWeakObjectPtr<AActor> Actor : ActorList)
	{
		if (!Actor.IsValid())
		{
			continue;
		}

		if (UStatusComponent* StatusComponent = UStatusInterfaceStatics::GetStatusComponent(Actor.Get()))
		{
			StatusComponent->OnDied.RemoveDynamic(this, &UObjectiveDestroyActor::OnActorDied);
		}

		Actor->OnDestroyed.RemoveDynamic(this, &UObjectiveDestroyActor::CharacterDestroyed);
	}
}

void UObjectiveDestroyActor::CharacterDestroyed(AActor* Character)
{
	if (UStatusComponent* StatusComponent = UStatusInterfaceStatics::GetStatusComponent(Character))
	{
		OnActorDied(StatusComponent, 0.f, FDamageEvent(), nullptr, nullptr);
	}
}

void UObjectiveDestroyActor::OnActorDied(UStatusComponent* Component, float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (ActorList.Num() == 0)
	{
		SetObjectiveProgress(0.f);
		return;
	}

	RemainingActorList.Remove(Component->GetOwner());
	UpdateObjectiveProgress();
}

void UObjectiveDestroyActor::UpdateObjectiveProgress()
{
	SetObjectiveProgress(float(ActorList.Num() - RemainingActorList.Num()) / float(ActorList.Num()));

	if (GetObjecitveProgress() == 1.f)
	{
		Complete();
	}
}