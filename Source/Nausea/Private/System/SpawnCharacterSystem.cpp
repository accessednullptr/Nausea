// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "System/SpawnCharacterSystem.h"
#include "System/CoreGameState.h"
#include "Character/CoreCharacter.h"

USpawnCharacterSystem::USpawnCharacterSystem(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

inline USpawnCharacterSystem* GetSpawnCharacterSystem(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World)
	{
		return nullptr;
	}

	ACoreGameState* CoreGameState = World->GetGameState<ACoreGameState>();

	if (!CoreGameState)
	{
		return nullptr;
	}

	return CoreGameState->GetSpawnCharacterSystem();
}

bool USpawnCharacterSystem::RequestSpawn(const UObject* WorldContextObject, TSubclassOf<ACoreCharacter> CoreCharacterClass, const FTransform& Transform, const FActorSpawnParameters& SpawnParameters, FCharacterSpawnRequestDelegate&& Delegate)
{
	USpawnCharacterSystem* SpawnCharacterSystem = GetSpawnCharacterSystem(WorldContextObject);
	
	if (!SpawnCharacterSystem)
	{
		return false;
	}

	return SpawnCharacterSystem->AddRequest(FSpawnRequest(CoreCharacterClass, Transform, SpawnParameters, MoveTemp(Delegate)));
}

int32 USpawnCharacterSystem::CancelRequestsForObject(const UObject* WorldContextObject, const UObject* OwningObject)
{
	if (!OwningObject)
	{
		return 0;
	}

	USpawnCharacterSystem* SpawnCharacterSystem = GetSpawnCharacterSystem(WorldContextObject);

	if (!SpawnCharacterSystem)
	{
		return 0;
	}

	return SpawnCharacterSystem->CancelRequestForObject(OwningObject);
}

int32 USpawnCharacterSystem::CancelAllRequests(const UObject* WorldContextObject)
{
	USpawnCharacterSystem* SpawnCharacterSystem = GetSpawnCharacterSystem(WorldContextObject);

	if (!SpawnCharacterSystem)
	{
		return 0;
	}

	return SpawnCharacterSystem->CancelAllRequests();
}

bool USpawnCharacterSystem::AddRequest(FSpawnRequest&& SpawnRequest)
{
	CharacterSpawnRequestList.Add(MoveTemp(SpawnRequest));

	if (GetWorld()->GetTimerManager().IsTimerActive(SpawnTimerHandle))
	{
		return true;
	}

	TWeakObjectPtr<USpawnCharacterSystem> WeakThis = this;
	GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, FTimerDelegate::CreateWeakLambda(this, [WeakThis]()
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->PerformNextSpawn();
		}), 0.075f, false);
	return true;
}

int32 USpawnCharacterSystem::CancelRequestForObject(const UObject* OwningObject)
{
	int32 NumCancelled = 0;
	for (int32 Index = CharacterSpawnRequestList.Num() - 1; Index >= 0; Index--)
	{
		if (!CharacterSpawnRequestList[Index].IsOwnedBy(OwningObject))
		{
			continue;
		}

		NumCancelled++;
		CharacterSpawnRequestList.RemoveAt(Index, 0, false);
	}

	return NumCancelled;
}

int32 USpawnCharacterSystem::CancelAllRequests()
{
	const int32 NumRequests = CharacterSpawnRequestList.Num();
	CharacterSpawnRequestList.Reset();
	return NumRequests;
}

void USpawnCharacterSystem::PerformNextSpawn()
{
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);

	if (CharacterSpawnRequestList.Num() <= 0 || !GetWorld())
	{
		return;
	}

	{
		FSpawnRequest* Request = nullptr;

		while ((!Request || !Request->IsValid()) && CharacterSpawnRequestList.Num() > 0)
		{
			Request = &CharacterSpawnRequestList[0];

			if (!Request->IsValid())
			{
				Request = nullptr;
				CharacterSpawnRequestList.RemoveAt(0, 1, false);
			}
		}

		const TSubclassOf<ACoreCharacter>& SpawnClass = Request->GetCharacterClass();

		if (SpawnClass == nullptr)
		{
			Request->BroadcastRequestResult(nullptr);
		}

		const FTransform& SpawnTransform = Request->GetTransform();
		const FActorSpawnParameters& ActorSpawnParams = Request->GetActorSpawnParameters();

		ACoreCharacter* Character = GetWorld()->SpawnActor<ACoreCharacter>(SpawnClass, SpawnTransform, ActorSpawnParams);

		Request->BroadcastRequestResult(Character);
		CharacterSpawnRequestList.RemoveAt(0, 1, false);
	}

	if (CharacterSpawnRequestList.Num() <= 0)
	{
		return;
	}

	TWeakObjectPtr<USpawnCharacterSystem> WeakThis = this;
	GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, FTimerDelegate::CreateWeakLambda(this, [WeakThis]()
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->PerformNextSpawn();
		}), 0.075f, false);
}