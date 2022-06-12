// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "System/SpawnCharacterSystem.h"
#include "System/CoreGameState.h"
#include "System/CoreGameMode.h"
#include "Character/CoreCharacter.h"

bool FSpawnRequest::IsValid() const
{
	if (!CharacterClass && !SpawnDelegate.IsBound())
	{
		return false;
	}

	return true;
}

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

bool USpawnCharacterSystem::SpawnCharacter(const UObject* WorldContextObject, TSubclassOf<ACoreCharacter> CoreCharacterClass, const FTransform& Transform, AActor* Owner, APawn* Instigator)
{
	USpawnCharacterSystem* SpawnCharacterSystem = GetSpawnCharacterSystem(WorldContextObject);

	if (!SpawnCharacterSystem)
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = Owner;
	SpawnParameters.Instigator = Instigator;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	return SpawnCharacterSystem->AddRequest(FSpawnRequest(CoreCharacterClass, Transform, SpawnParameters));
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
		}), 0.05f, false);
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

		//If this broadcast was unhandled, manually notify the game mode of this spawn ourselves (otherwise expect the binding to handle it).
		if (!Request->BroadcastRequestResult(Character))
		{
			if (ACoreGameMode* GameMode = GetWorld()->GetAuthGameMode<ACoreGameMode>())
			{
				GameMode->SetPlayerDefaults(Character);
			}
		}
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
		}), 0.025f, false);
}

USpawnLocationInterface::USpawnLocationInterface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

USpawnLocationSystemLibrary::USpawnLocationSystemLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

bool USpawnLocationSystemLibrary::GetSpawnLocation(TScriptInterface<ISpawnLocationInterface> Target, TSubclassOf<ACoreCharacter> CoreCharacter, FTransform& SpawnTransform)
{
	if (!Target)
	{
		return false;
	}

	return Target->GetSpawnTransform(CoreCharacter, SpawnTransform);
}