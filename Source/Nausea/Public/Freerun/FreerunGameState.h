// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "System/NauseaGameState.h"
#include "FreerunGameState.generated.h"

class UObjectiveComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FObjectiveChangedSignature, UMissionComponent*, Objective, UMissionComponent*, PreviousObjective);

/**
 * 
 */
UCLASS()
class NAUSEA_API AFreerunGameState : public ANauseaGameState
{
	GENERATED_UCLASS_BODY()

//~ Begin AGameState Interface
protected:
	virtual void HandleMatchHasStarted() override;
//~ End AGameState Interface

public:
	UFUNCTION()
	virtual void ResetLevel();
	UFUNCTION(BlueprintPure, Category = FreerunGameState)
	bool IsResettingLevel() const { return bIsResetting; }

protected:
	UFUNCTION(Reliable, NetMulticast)
	void Multicast_Reliable_ResetLevel();

protected:
	UPROPERTY()
	bool bIsResetting = false;
};
