// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "AITypes.h"
#include "AI/RoutineManager/Routine.h"
#include "RoutineActionPushAction.generated.h"

class UActionBrainComponentAction;

/**
 * 
 */
UCLASS()
class NAUSEA_API URoutineActionPushAction : public URoutineAction
{
	GENERATED_UCLASS_BODY()
	
//~ Begin URoutineAction Interface
public:
	virtual void StartAction() override;
	virtual void EndAction() override;
	virtual FString DescribeRoutineActionToGameplayDebugger() const override;
//~ Begin URoutineAction Interface

protected:
	UFUNCTION()
	virtual UActionBrainComponentAction* CreateAction();
	UFUNCTION(BlueprintImplementableEvent, Category = Routine)
	void K2_OnActionCreated(UActionBrainComponentAction* Action);

	UFUNCTION()
	void OnActionFinished(UActionBrainComponentAction* Action, TEnumAsByte<EPawnActionResult::Type> Result);

protected:
	UPROPERTY(EditDefaultsOnly, Category = Routine)
	TSubclassOf<UActionBrainComponentAction> ActionClass = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Routine, Instanced)
	UActionBrainDataObject* ActionDataObject = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Routine)
	TEnumAsByte<EAIRequestPriority::Type> Priority = EAIRequestPriority::Logic;
	UPROPERTY(EditDefaultsOnly, Category = Routine)
	bool bLoopAction = true;
	UPROPERTY(EditDefaultsOnly, Category = Routine, meta = (EditCondition = "bLoopAction"))
	bool bEndOnActionFailure = false;

	UPROPERTY()
	UActionBrainComponentAction* CurrentAction = nullptr;
};