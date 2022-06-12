// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Templates/SubclassOf.h"
#include "WaveSpawnLocationInterface.generated.h"

class ACoreCharacter;

UINTERFACE(Blueprintable)
class UWaveSpawnLocationInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

/**
 * 
 */
class NAUSEA_API IWaveSpawnLocationInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	//Returns a spawn location via FTransform for a given SpawnClass. Returns FTransform::Identity if unable to find one.
	//int32& RequestID is meant to allow a given spawner to maintain some sort of data during spawn request iterator.
	//-1 will be passed if a random spawn is requested.
	UFUNCTION()
	virtual FTransform GetSpawnLocation(TSubclassOf<ACoreCharacter> SpawnClass, int32& RequestID) const { PURE_VIRTUAL(IWaveSpawnLocationInterface::GetSpawnLocation, return FTransform::Identity;) }

	//Called when a character is spawned using the object that implements this interface.
	UFUNCTION()
	virtual void ProcessSpawn(ACoreCharacter* SpawnedCharacter, int32& RequestID) { }

	//Returns the score of this spawn location. The higher the score, the more desirable the spawn location.
	UFUNCTION()
	virtual float GetSpawnScore() const { PURE_VIRTUAL(IWaveSpawnLocationInterface::GetSpawnScore, return -1.f;) }
};
