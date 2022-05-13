// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Perception/AIPerceptionSystem.h"
#include "CoreAIPerceptionSystem.generated.h"

class AActor;

/**
 * 
 */
UCLASS()
class NAUSEA_API UCoreAIPerceptionSystem : public UAIPerceptionSystem
{
	GENERATED_BODY()
	
public:
	static UCoreAIPerceptionSystem* GetCoreCurrent(UObject* WorldContextObject);
	static UCoreAIPerceptionSystem* GetCoreCurrent(UWorld& World);

	bool IsActorRegisteredStimuliSource(const AActor* Actor) const { return RegisteredStimuliSources.Contains(Actor); }
	const FPerceptionStimuliSource& GetPerceptionStimiliSourceForActor(const AActor* Actor) const;
	bool DoesActorHaveStimuliSourceSenseID(const AActor* Actor, FAISenseID SenseID) const;
};
