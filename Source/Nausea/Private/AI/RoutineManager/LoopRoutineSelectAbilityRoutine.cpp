// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "AI/RoutineManager/LoopRoutineSelectAbilityRoutine.h"
#include "AI/RoutineManager/RoutineActionPushAction.h"
#include "AI/Action/ActionPerformAbility.h"
#include "Gameplay/AbilityComponent.h"

bool FRoutineAbilityEntry::IsValid() const
{
	return AbilityClass != nullptr;
}

bool FRoutineAbilityEntry::CanBeSelected(const UAbilityComponent* AbilityComponent) const
{
	return AbilityClass != nullptr && AbilityComponent && AbilityComponent->CanPerformAbility(AbilityClass) == EAbilityRequestResponse::Success;
}

ULoopRoutineSelectAbilityRoutine::ULoopRoutineSelectAbilityRoutine(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultRoutineAction = URoutineActionPushAction::StaticClass();
}

void ULoopRoutineSelectAbilityRoutine::StartRoutine()
{
	IPlayerOwnershipInterface* PlayerOwnershipInterface = Cast<IPlayerOwnershipInterface>(GetAIController());
	if (PlayerOwnershipInterface)
	{
		CachedAbilityComponent = PlayerOwnershipInterface->GetAbilityComponent();
	}

	TArray<TSubclassOf<UAbilityInfo>> KeyArray;
	RoutineAbilityMap.GenerateKeyArray(KeyArray);
	for (TSubclassOf<UAbilityInfo>& Key : KeyArray)
	{
		RoutineAbilityMap[Key].SetClass(Key);
	}

	Super::StartRoutine();
}

FString ULoopRoutineSelectAbilityRoutine::DescribeRoutineToGameplayDebugger() const
{
	if (GetCurrentRoutineAction())
	{
		return FString::Printf(TEXT("{yellow} %s {white}[loop %i]\n    %s%s"),
			*GetName(), LoopCount,
			*GetCurrentRoutineAction()->DescribeRoutineActionToGameplayDebugger(), *DescribeAbilityMapToGameplayDebugger());
	}

	return FString::Printf(TEXT("{yellow} %s {white}[loop %i]%s"), *GetName(), LoopCount, *DescribeAbilityMapToGameplayDebugger());
}

URoutineAction* ULoopRoutineSelectAbilityRoutine::CreateRoutineAction(TSubclassOf<URoutineAction> RoutineActionClass, bool bAutoStart)
{
	if (!NextSelectedAbility.IsValid())
	{
		return nullptr;
	}

	URoutineAction* RoutineAction = Super::CreateRoutineAction(RoutineActionClass, false);

	if (!RoutineAction)
	{
		return nullptr;
	}

	if (URoutineActionPushAbilityAction* PushActionRoutine = Cast<URoutineActionPushAbilityAction>(RoutineAction))
	{
		PushActionRoutine->SetAbilityClass(NextSelectedAbility.GetAbilityClass());
		NextSelectedAbility = FRoutineAbilityEntry();
	}

	if (bAutoStart)
	{
		RoutineAction->StartAction();
	}

	return RoutineAction;
}

TSubclassOf<URoutineAction> ULoopRoutineSelectAbilityRoutine::GetNextRoutineAction()
{
	if (!CachedAbilityComponent.IsValid())
	{
		return nullptr;
	}

	TArray<FRoutineAbilityEntry*> BestAbilityEntryList;

	for (TPair<TSubclassOf<UAbilityInfo>, FRoutineAbilityEntry>& Entry : RoutineAbilityMap)
	{
		FRoutineAbilityEntry& Value = Entry.Value;

		if (!Value.CanBeSelected(CachedAbilityComponent.Get()))
		{
			continue;
		}

		if (BestAbilityEntryList.Num() == 0)
		{
			BestAbilityEntryList.Add(&Value);
			continue;
		}

		if (Value.GetPriority() > BestAbilityEntryList[0]->GetPriority())
		{
			BestAbilityEntryList.Empty();
			BestAbilityEntryList.Add(&Value);
			continue;
		}
		else if (Value.GetPriority() == BestAbilityEntryList[0]->GetPriority())
		{
			BestAbilityEntryList.Add(&Value);
			continue;
		}
	}

	if (BestAbilityEntryList.Num() == 0)
	{
		return nullptr;
	}

	if (BestAbilityEntryList.Num() == 1)
	{
		NextSelectedAbility = *BestAbilityEntryList[0];
		return DefaultRoutineAction;
	}

	float CumulativeWeight = 0.f;
	for (FRoutineAbilityEntry* Entry : BestAbilityEntryList)
	{
		CumulativeWeight += Entry->GetWeight();
	}

	CumulativeWeight = FMath::RandRange(0.f, CumulativeWeight);
	for (FRoutineAbilityEntry* Entry : BestAbilityEntryList)
	{
		CumulativeWeight -= Entry->GetWeight();

		if (CumulativeWeight <= 0.f)
		{
			NextSelectedAbility = *Entry;
			break;
		}
	}

	if (!NextSelectedAbility.IsValid())
	{
		return nullptr;
	}

	return DefaultRoutineAction;
}

FString ULoopRoutineSelectAbilityRoutine::DescribeAbilityMapToGameplayDebugger() const
{
	FString Description = "";
	for (const TPair<TSubclassOf<UAbilityInfo>, FRoutineAbilityEntry>& Entry : RoutineAbilityMap)
	{
		Description += FString::Printf(TEXT("\n    {yellow}%s"), *GetNameSafe(Entry.Key));
	}

	return Description;
}