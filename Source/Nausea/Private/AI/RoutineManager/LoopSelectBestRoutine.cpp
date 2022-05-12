// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "AI/RoutineManager/LoopSelectBestRoutine.h"

bool FRoutineActionEntry::NotifyCompleted(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (ExecutionLimit > 0)
	{
		ExecutionCount++;
	}

	if (!World)
	{
		return false;
	}

	if (CooldownDuration > 0.f)
	{
		World->GetTimerManager().SetTimer(CooldownTimerHandle, CooldownDuration, false);
	}

	return true;
}

bool FRoutineActionEntry::IsOnCooldown(const UObject* WorldContextObject) const
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World)
	{
		return false;
	}

	return World->GetTimerManager().IsTimerActive(CooldownTimerHandle);
}

float FRoutineActionEntry::GetCooldownRemaining(const UObject* WorldContextObject) const
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World || !World->GetTimerManager().IsTimerActive(CooldownTimerHandle))
	{
		return 0.f;
	}

	return World->GetTimerManager().GetTimerRemaining(CooldownTimerHandle);
}

ULoopSelectBestRoutine::ULoopSelectBestRoutine(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void ULoopSelectBestRoutine::StartRoutine()
{
	TArray<TSubclassOf<URoutineAction>> KeyArray;
	RoutineActionMap.GenerateKeyArray(KeyArray);
	for (TSubclassOf<URoutineAction>& Key : KeyArray)
	{
		RoutineActionMap[Key].SetClass(Key);
	}

	Super::StartRoutine();
}

void ULoopSelectBestRoutine::RoutineActionCompleted(URoutineAction* RoutineAction)
{
	if (RoutineAction && RoutineActionMap.Contains(RoutineAction->GetClass()))
	{
		RoutineActionMap[RoutineAction->GetClass()].NotifyCompleted(this);
	}

	Super::RoutineActionCompleted(RoutineAction);
}

FString ULoopSelectBestRoutine::DescribeRoutineToGameplayDebugger() const
{
	if (GetCurrentRoutineAction())
	{
		return FString::Printf(TEXT("{yellow} %s {white}[loop %i]\n    %s%s"),
			*GetName(), LoopCount,
			*GetCurrentRoutineAction()->DescribeRoutineActionToGameplayDebugger(), *DescribeActionMapToGameplayDebugger());
	}

	return FString::Printf(TEXT("{yellow} %s {white}[loop %i]%s"), *GetName(), LoopCount, *DescribeActionMapToGameplayDebugger());
}

TSubclassOf<URoutineAction> ULoopSelectBestRoutine::GetNextRoutineAction()
{
	TArray<FRoutineActionEntry*> BestActionEntryList;

	for (TPair<TSubclassOf<URoutineAction>, FRoutineActionEntry>& Entry : RoutineActionMap)
	{
		FRoutineActionEntry& Value = Entry.Value;

		if (!Value.CanBeSelected(this))
		{
			continue;
		}

		if (BestActionEntryList.Num() == 0)
		{
			BestActionEntryList.Add(&Value);
			continue;
		}

		if (Value.GetPriority() > BestActionEntryList[0]->GetPriority())
		{
			BestActionEntryList.Empty();
			BestActionEntryList.Add(&Value);
			continue;
		}
		else if (Value.GetPriority() == BestActionEntryList[0]->GetPriority())
		{
			BestActionEntryList.Add(&Value);
			continue;
		}
	}

	if (BestActionEntryList.Num() == 0)
	{
		return DefaultRoutineAction;
	}

	if (BestActionEntryList.Num() == 1)
	{
		return BestActionEntryList[0]->GetRoutineActionClass();
	}

	float CumulativeWeight = 0.f;
	for (FRoutineActionEntry* Entry : BestActionEntryList)
	{
		CumulativeWeight += Entry->GetWeight();
	}

	CumulativeWeight = FMath::RandRange(0.f, CumulativeWeight);
	for (FRoutineActionEntry* Entry : BestActionEntryList)
	{
		CumulativeWeight -= Entry->GetWeight();

		if (CumulativeWeight <= 0.f)
		{
			return Entry->GetRoutineActionClass();
		}
	}

	return DefaultRoutineAction;
}

FString ULoopSelectBestRoutine::DescribeActionMapToGameplayDebugger() const
{
	FString Description = "";

	TArray<TSubclassOf<URoutineAction>> KeyArray;
	RoutineActionMap.GenerateKeyArray(KeyArray);
	for (TSubclassOf<URoutineAction>& Key : KeyArray)
	{
		const FRoutineActionEntry& Entry = RoutineActionMap[Key];
		
		const FString ClassName = Key->GetName();
		const FString Cooldown = Entry.IsOnCooldown(this) ? FString("{red}") + FString::FromInt(FMath::CeilToInt(Entry.GetCooldownRemaining(this))) : FString("0");
		const FString RemainingUses = Entry.GetRemainingExecutions() != -1 ? FString("{red}") + FString::FromInt(Entry.GetRemainingExecutions()) : FString("Unlimited");

		Description += FString::Printf(TEXT("\n    {yellow}%s\n  {white}Cooldown: %s\n  {white}Remaning Uses: %s"), *ClassName, *Cooldown, *RemainingUses);
	}

	return Description;
}