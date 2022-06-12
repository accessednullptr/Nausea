// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "System/CoreSingleton.h"
#include "Engine/GameInstance.h"
#include "GameFramework/WorldSettings.h"

UCoreSingleton::UCoreSingleton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UCoreSingleton::Tick(float DeltaTime)
{
	if (GameInstance.IsValid() && GameInstance->GetWorld() && GameInstance->GetWorld()->GetWorldSettings())
	{
		DeltaTime *= GameInstance->GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation();
	}

	for (int32 Index = SingletonTickCallbackList.Num() - 1; Index >= 0; Index--)
	{
		SingletonTickCallbackList[Index].ExecuteIfBound(DeltaTime);
	}
}

inline UCoreSingleton* GetSingleton(const UObject* WorldContextObject)
{
	if (!GEngine)
	{
		return nullptr;
	}

	return Cast<UCoreSingleton>(GEngine->GameSingleton);
}

void UCoreSingleton::BindToSingletonTick(const UObject* WorldContextObject, FOnSingletonTickSignature Delegate)
{
	if (UCoreSingleton* Singleton = GetSingleton(WorldContextObject))
	{
		if (!Singleton->GameInstance.IsValid())
		{
			if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
			{
				Singleton->GameInstance = World->GetGameInstance();
			}
		}

		Singleton->SingletonTickCallbackList.AddUnique(Delegate);
	}
}

void UCoreSingleton::UnbindFromSingletonTick(const UObject* WorldContextObject, FOnSingletonTickSignature Delegate)
{
	if (UCoreSingleton* Singleton = GetSingleton(WorldContextObject))
	{
		Singleton->SingletonTickCallbackList.Remove(Delegate);
	}
}

void UCoreSingleton::RegisterActorWithTag(const UObject* WorldContextObject, AActor* Actor, const FGameplayTagContainer& Tag)
{
	if (!Actor)
	{
		return;
	}

	if (UCoreSingleton* Singleton = GetSingleton(WorldContextObject))
	{
		for (const FGameplayTag& IndividualTag : Tag)
		{
			Singleton->ActorGameplayTagMap.FindOrAdd(IndividualTag).Add(Actor);
		}
	}
}

void UCoreSingleton::UnregisterActorWithTag(const UObject* WorldContextObject, AActor* Actor, const FGameplayTagContainer& Tag)
{
	if (!Actor)
	{
		return;
	}

	if (UCoreSingleton* Singleton = GetSingleton(WorldContextObject))
	{
		TArray<FGameplayTag> RemovableTagEntryList;

		for (const FGameplayTag& IndividualTag : Tag)
		{
			if (!Singleton->ActorGameplayTagMap.Contains(IndividualTag))
			{
				continue;
			}

			TArray<TWeakObjectPtr<AActor>>& TaggedActorList = Singleton->ActorGameplayTagMap[IndividualTag];
			TaggedActorList.Remove(Actor);

			if (TaggedActorList.Num() == 0)
			{
				RemovableTagEntryList.Add(IndividualTag);
			}
		}

		for (const FGameplayTag& IndividualTag : RemovableTagEntryList)
		{
			Singleton->ActorGameplayTagMap.Remove(IndividualTag);
		}
	}
}

void UCoreSingleton::GetActorsWithTag(const UObject* WorldContextObject, TArray<AActor*>& ActorList, const FGameplayTagContainer& Tag)
{
	ActorList.Reset();
	
	if (UCoreSingleton* Singleton = GetSingleton(WorldContextObject))
	{
		for (const FGameplayTag& IndividualTag : Tag)
		{
			if (!Singleton->ActorGameplayTagMap.Contains(IndividualTag))
			{
				continue;
			}

			const TArray<TWeakObjectPtr<AActor>>& TaggedActorList = Singleton->ActorGameplayTagMap[IndividualTag];
			ActorList.Reserve(ActorList.Num() + TaggedActorList.Num());

			for (const TWeakObjectPtr<AActor>& Actor : TaggedActorList)
			{
				if (!Actor.IsValid())
				{
					continue;
				}

				ActorList.Add(Actor.Get());
			}
		}
	}
}