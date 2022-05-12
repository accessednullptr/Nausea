// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "AI/RoutineManagerComponent.h"
#include "AI/RoutineManager/Routine.h"
#include "AI/CoreAIController.h"
#include "AI/EnemySelectionComponent.h"

URoutineManagerComponent::URoutineManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void URoutineManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	const bool bMainEventNotBound = GetAIController() ? !GetAIController()->OnPawnUpdated.IsAlreadyBound(this, &URoutineManagerComponent::OnPawnUpdated) : false;

	if (bMainEventNotBound)
	{
		return;
	}

	if (!GetAIController()->GetEnemySelectionComponent())
	{
		return;
	}

	GetAIController()->GetEnemySelectionComponent()->OnEnemyChanged.AddDynamic(this, &URoutineManagerComponent::EnemyChanged);
}

void URoutineManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	const bool bMainEventNotBound = GetAIController() ? !GetAIController()->OnPawnUpdated.IsAlreadyBound(this, &URoutineManagerComponent::OnPawnUpdated) : true;

	Super::EndPlay(EndPlayReason);

	if (bMainEventNotBound)
	{
		return;
	}

	GetAIController()->GetEnemySelectionComponent()->OnEnemyChanged.RemoveDynamic(this, &URoutineManagerComponent::EnemyChanged);
}

void URoutineManagerComponent::OnPawnUpdated(ACoreAIController* AIController, ACoreCharacter* InCharacter)
{
	Super::OnPawnUpdated(AIController, InCharacter);

	if (!InCharacter)
	{
		EndRoutine();
		return;
	}

	if (DefaultRoutineClass)
	{
		CreateRoutine(DefaultRoutineClass);
	}
}

URoutineAction* URoutineManagerComponent::GetCurrentRoutineAction() const
{
	return GetCurrentRoutine() ? GetCurrentRoutine()->GetCurrentRoutineAction() : nullptr;
}

URoutine* URoutineManagerComponent::CreateRoutine(TSubclassOf<URoutine> RoutineClass, bool bAutoStart)
{
	if (!RoutineClass)
	{
		return nullptr;
	}

	URoutine* Routine = NewObject<URoutine>(this, RoutineClass);

	if (bAutoStart)
	{
		StartRoutine(Routine);
	}
	
	return Routine;
}

void URoutineManagerComponent::StartRoutine(URoutine* Routine)
{
	if (IsPendingKill())
	{
		return;
	}

	if (CurrentRoutine)
	{
		EndRoutine();
	}

	if (!Routine)
	{
		return;
	}

	ensure(Routine && !Routine->IsPendingKill());

	CurrentRoutine = Routine;
	BindToRoutine(CurrentRoutine);
	CurrentRoutine->StartRoutine();
}

void URoutineManagerComponent::EndRoutine()
{
	if (!CurrentRoutine)
	{
		return;
	}

	CurrentRoutine->EndRoutine();
}

FString URoutineManagerComponent::DescribeRoutineManagerToGameplayDebugger() const
{
	if (!CurrentRoutine)
	{
		return "{red}No Routine Selected";
	}

	return CurrentRoutine->DescribeRoutineToGameplayDebugger();
}

void URoutineManagerComponent::BindToRoutine(URoutine* Routine)
{
	if (!Routine || Routine->OnRoutineComplete.IsAlreadyBound(this, &URoutineManagerComponent::RoutineCompleted))
	{
		return;
	}

	Routine->OnRoutineComplete.AddDynamic(this, &URoutineManagerComponent::RoutineCompleted);
	Routine->OnRoutineActionComplete.AddDynamic(this, &URoutineManagerComponent::RoutineActionCompleted);
}

void URoutineManagerComponent::UnbindToRoutine(URoutine* Routine)
{
	if (!Routine || !Routine->OnRoutineComplete.IsAlreadyBound(this, &URoutineManagerComponent::RoutineCompleted))
	{
		return;
	}

	Routine->OnRoutineComplete.RemoveDynamic(this, &URoutineManagerComponent::RoutineCompleted);
	Routine->OnRoutineActionComplete.RemoveDynamic(this, &URoutineManagerComponent::RoutineActionCompleted);
}

void URoutineManagerComponent::RoutineCompleted(URoutine* Routine)
{
	if (!Routine || Routine->GetRoutineManager() != this)
	{
		return;
	}

	UnbindToRoutine(Routine);
	OnRoutineComplete.Broadcast(this, Routine);
	K2_RoutineCompleted(Routine);
	
	if (Routine == CurrentRoutine)
	{
		CurrentRoutine = nullptr;
	}
}

void URoutineManagerComponent::EnemyChanged(UEnemySelectionComponent* EnemySelectionComponent, AActor* NewEnemy, AActor* PreviousEnemy)
{
	K2_EnemyChanged(EnemySelectionComponent, NewEnemy, PreviousEnemy);
}

void URoutineManagerComponent::RoutineActionCompleted(URoutine* Routine, URoutineAction* RoutineAction)
{
	if (!Routine || Routine->GetRoutineManager() != this)
	{
		return;
	}

	OnRoutineActionComplete.Broadcast(this, Routine, RoutineAction);
}