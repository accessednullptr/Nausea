// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "AI/RoutineManager/Routine.h"
#include "LoopRoutine.generated.h"

class URoutineAction;

UENUM(BlueprintType)
enum class ENextRoutineNullResponse : uint8
{
	EndRoutine,
	RetryNextFrame
};

/**
 * 
 */
UCLASS()
class NAUSEA_API ULoopRoutine : public URoutine
{
	GENERATED_UCLASS_BODY()
	
//~ Begin URoutine Interface
public:
	virtual void StartRoutine() override;
protected:
	virtual void RoutineActionCompleted(URoutineAction* RoutineAction) override;
//~ End URoutine Interface

protected:
	UFUNCTION()
	void StartNextRoutineAction();
	UFUNCTION()
	virtual TSubclassOf<URoutineAction> GetNextRoutineAction();

protected:
	UPROPERTY(EditDefaultsOnly, Category = Routine)
	ENextRoutineNullResponse NextRoutineNullResponse = ENextRoutineNullResponse::RetryNextFrame;

	//Default routine action to loop.
	UPROPERTY(EditDefaultsOnly, Category = Routine)
	TSubclassOf<URoutineAction> DefaultRoutineAction = nullptr;

	uint32 LoopCount = 0;
};
