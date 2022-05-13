// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "AI/CoreAIPerceptionSystem.h"

UCoreAIPerceptionSystem* UCoreAIPerceptionSystem::GetCoreCurrent(UObject* WorldContextObject)
{
	UWorld* World = Cast<UWorld>(WorldContextObject);
	if (World == nullptr && WorldContextObject != nullptr)
	{
		World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	}

	if (World && World->GetAISystem())
	{
		UAISystem* AISys = CastChecked<UAISystem>(World->GetAISystem());
		return CastChecked<UCoreAIPerceptionSystem>(AISys->GetPerceptionSystem());
	}

	return nullptr;
}

UCoreAIPerceptionSystem* UCoreAIPerceptionSystem::GetCoreCurrent(UWorld& World)
{
	if (World.GetAISystem())
	{
		check(Cast<UAISystem>(World.GetAISystem()));
		UAISystem* AISys = (UAISystem*)(World.GetAISystem());
		return CastChecked<UCoreAIPerceptionSystem>(AISys->GetPerceptionSystem());
	}

	return nullptr;
}

const FPerceptionStimuliSource& UCoreAIPerceptionSystem::GetPerceptionStimiliSourceForActor(const AActor* Actor) const
{
	static FPerceptionStimuliSource InvalidPerceptionStimuliSource = FPerceptionStimuliSource();

	if (Actor && RegisteredStimuliSources.Contains(Actor))
	{
		return RegisteredStimuliSources[Actor];
	}

	return InvalidPerceptionStimuliSource;
}

bool UCoreAIPerceptionSystem::DoesActorHaveStimuliSourceSenseID(const AActor* Actor, FAISenseID SenseID) const
{
	if (Actor && RegisteredStimuliSources.Contains(Actor))
	{
		return RegisteredStimuliSources[Actor].RelevantSenses.ShouldRespondToChannel(SenseID);
	}

	return false;
}