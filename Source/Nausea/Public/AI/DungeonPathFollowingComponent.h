// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "AI/CorePathFollowingComponent.h"
#include "DungeonPathFollowingComponent.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API UDungeonPathFollowingComponent : public UCorePathFollowingComponent
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UActorComponent Interface
public:
	virtual void BeginPlay() override;
//~ End UActorComponent Interface

//~ Begin UPathFollowingComponent Interface
public:
	virtual void OnLanded() override;
	virtual void OnPathFinished(const FPathFollowingResult& Result) override;
//~ End UPathFollowingComponent Interface

protected:
	UFUNCTION()
	void OnPathFailure();
	UFUNCTION()
	void DecrementPathFailureCounter();

protected:
	UPROPERTY(Transient)
	uint8 PathFailureCount = 0;
	UPROPERTY(Transient)
	FTimerHandle PathFailureTimer;
};
