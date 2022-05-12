// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#include "AI/RoutineManager/Routine.h"
#include "AI/CoreAIController.h"
#include "AI/RoutineManagerComponent.h"
#include "UObject/ReferenceChainSearch.h"

URoutine::URoutine(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void URoutine::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		return;
	}

	RoutineManager = GetTypedOuter<URoutineManagerComponent>();
	ensure(RoutineManager);
	AIController = GetTypedOuter<ACoreAIController>();
	ensure(AIController);
	WorldPrivate = AIController->GetWorld();
}

void URoutine::BeginDestroy()
{
	WorldPrivate = nullptr;
	CurrentRoutineAction = nullptr;
	RoutineManager = nullptr;
	AIController = nullptr;

	Super::BeginDestroy();
}

UWorld* URoutine::GetWorld_Uncached() const
{
	UWorld* ObjectWorld = nullptr;

	const UObject* Owner = GetRoutineManager();

	if (Owner && !Owner->HasAnyFlags(RF_ClassDefaultObject))
	{
		ObjectWorld = Owner->GetWorld();
	}

	if (ObjectWorld == nullptr)
	{
		if (AActor* Actor = GetTypedOuter<AActor>())
		{
			ObjectWorld = Actor->GetWorld();
		}
		else
		{
			ObjectWorld = Cast<UWorld>(GetOuter());
		}
	}

	return ObjectWorld;
}

void URoutine::StartRoutine()
{
	if (IsRoutineStarted())
	{
		return;
	}

	K2_StartRoutine();

	bRoutineStarted = true;
	bRoutineCompleted = false;
}

void URoutine::EndRoutine()
{
	if (!IsRoutineStarted() || IsRoutineCompleted())
	{
		return;
	}

	K2_EndRoutine();

	bRoutineCompleted = true;
	bRoutineStarted = false;

	if (CurrentRoutineAction)
	{
		CurrentRoutineAction->EndAction();
	}

	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	OnRoutineComplete.Broadcast(this);
	//MarkPendingKill();
}

FString URoutine::DescribeRoutineToGameplayDebugger() const
{
	if (CurrentRoutineAction)
	{
		return FString::Printf(TEXT("{yellow} %s\n    %s"),
			*GetName(),
			*CurrentRoutineAction->DescribeRoutineActionToGameplayDebugger());
	}

	return FString::Printf(TEXT("{yellow} %s"), *GetName());
}

void URoutine::BindToRoutineAction(URoutineAction* RoutineAction)
{
	if (!RoutineAction || RoutineAction->OnActionCompleted.IsAlreadyBound(this, &URoutine::RoutineActionCompleted))
	{
		return;
	}

	RoutineAction->OnActionCompleted.AddDynamic(this, &URoutine::RoutineActionCompleted);
}

void URoutine::UnbindToRoutineAction(URoutineAction* RoutineAction)
{
	if (!RoutineAction || !RoutineAction->OnActionCompleted.IsAlreadyBound(this, &URoutine::RoutineActionCompleted))
	{
		return;
	}

	RoutineAction->OnActionCompleted.RemoveDynamic(this, &URoutine::RoutineActionCompleted);
}

URoutineAction* URoutine::CreateRoutineAction(TSubclassOf<URoutineAction> RoutineActionClass, bool bAutoStart)
{
	if (!RoutineActionClass)
	{
		return nullptr;
	}
	
	URoutineAction* RoutineAction = NewObject<URoutineAction>(this, RoutineActionClass);
	ensure(RoutineAction && !RoutineAction->IsPendingKill());
	if (bAutoStart)
	{
		StartRoutineAction(RoutineAction);
	}

	return RoutineAction;
}

void URoutine::StartRoutineAction(URoutineAction* RoutineAction)
{
	if (CurrentRoutineAction)
	{
		StopCurrentRoutineAction();
	}

	ensure(RoutineAction && !RoutineAction->IsPendingKill());

	if (!RoutineAction)
	{
		return;
	}

	CurrentRoutineAction = RoutineAction;
	CurrentRoutineAction->StartAction();
	BindToRoutineAction(CurrentRoutineAction);
}

void URoutine::StopCurrentRoutineAction()
{
	UnbindToRoutineAction(CurrentRoutineAction);

	if (!CurrentRoutineAction->IsActionCompleted())
	{
		CurrentRoutineAction->EndAction();
	}

	CurrentRoutineAction = nullptr;
}

void URoutine::RoutineActionCompleted(URoutineAction* RoutineAction)
{
	if (!RoutineAction || RoutineAction->GetOwningRoutine() != this)
	{
		return;
	}

	StopCurrentRoutineAction();
	OnRoutineActionComplete.Broadcast(this, RoutineAction);
	K2_RoutineActionCompleted(RoutineAction);

	if (bEndRoutineOnActionComplete)
	{
		EndRoutine();
	}
}

URoutineAction::URoutineAction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void URoutineAction::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		return;
	}

	OwningRoutine = GetTypedOuter<URoutine>();
	ensure(OwningRoutine);
	AIController = OwningRoutine->GetTypedOuter<ACoreAIController>();
	ensure(AIController);

	WorldPrivate = AIController->GetWorld();
}

void URoutineAction::BeginDestroy()
{
	WorldPrivate = nullptr;
	OwningRoutine = nullptr;
	AIController = nullptr;

	Super::BeginDestroy();
}

UWorld* URoutineAction::GetWorld_Uncached() const
{
	UWorld* ObjectWorld = nullptr;

	const UObject* Owner = GetOwningRoutine()->GetRoutineManager();

	if (Owner && !Owner->HasAnyFlags(RF_ClassDefaultObject))
	{
		ObjectWorld = Owner->GetWorld();
	}

	if (ObjectWorld == nullptr)
	{
		if (AActor* Actor = GetTypedOuter<AActor>())
		{
			ObjectWorld = Actor->GetWorld();
		}
		else
		{
			ObjectWorld = Cast<UWorld>(GetOuter());
		}
	}

	return ObjectWorld;
}

void URoutineAction::StartAction()
{
	if (IsActionStarted())
	{
		return;
	}

	bActionStarted = true;

	K2_StartAction();
}

void URoutineAction::EndAction()
{
	if (!IsActionStarted() || IsActionCompleted())
	{
		return;
	}

	bActionCompleted = true;
	OnActionCompleted.Broadcast(this);
	K2_EndAction();

	//MarkPendingKill();
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

FString URoutineAction::DescribeRoutineActionToGameplayDebugger() const
{
	return FString::Printf(TEXT("{yellow}%s"), *GetName());
}

URoutinePushRoutineAction::URoutinePushRoutineAction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void URoutinePushRoutineAction::StartRoutine()
{
	Super::StartRoutine();

	CreateRoutineAction(DefaultRoutineAction, true);
}