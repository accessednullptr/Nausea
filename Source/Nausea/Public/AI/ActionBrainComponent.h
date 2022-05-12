// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BrainComponent.h"
#include "ActionStack.h"
#include "ActionBrainComponent.generated.h"

class ACoreAIController;
class ACoreCharacter;
class UActionBrainComponentAction;

USTRUCT()
struct NAUSEA_API FActionEvent
{
	GENERATED_USTRUCT_BODY()

	// used for marking FPawnActionEvent instances created solely for comparisons uses
	static const int32 FakeActionIndex = INDEX_NONE;

	UPROPERTY()
	UActionBrainComponentAction* Action;

	EPawnActionEventType::Type EventType;

	EAIRequestPriority::Type Priority;

	// used to maintain order of equally-important messages
	uint32 Index;

	FActionEvent() : Action(NULL), EventType(EPawnActionEventType::Invalid), Priority(EAIRequestPriority::MAX), Index(uint32(-1))
	{}

	FActionEvent(UActionBrainComponentAction* Action, EPawnActionEventType::Type EventType, uint32 Index);

	bool operator==(const FActionEvent& Other) const { return (Action == Other.Action) && (EventType == Other.EventType) && (Priority == Other.Priority); }
};

NAUSEA_API DECLARE_LOG_CATEGORY_EXTERN(LogActionBrain, Warning, All);

/**
 * UActionBrainComponent is a version of UPawnActionsComponent but is rewritten to be a more standardized UBrainComponent.
 */
UCLASS(BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class NAUSEA_API UActionBrainComponent : public UBrainComponent
{
	GENERATED_UCLASS_BODY()
	
public:
	UFUNCTION()
	ACoreCharacter* GetPawn() const { return Pawn; }

//~ Begin UActorComponent Interface
public:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
//~ End UActorComponent Interface

//~ Begin UBrainComponent Interface
public:
	virtual void StartLogic() override;
	virtual void RestartLogic() override;
	virtual bool IsRunning() const override;
	virtual bool IsPaused() const override;
	virtual void StopLogic(const FString& Reason) override;
	virtual void Cleanup() override;
	virtual void PauseLogic(const FString& Reason) override;
	virtual EAILogicResuming::Type ResumeLogic(const FString& Reason) override;
	virtual FString GetDebugInfoString() const override;
//~ End UBrainComponent Interface

public:
	UFUNCTION(BlueprintCallable, Category = ActionBrainComponent)
	bool PerformAction(UActionBrainComponentAction* Action, TEnumAsByte<EAIRequestPriority::Type> Priority = EAIRequestPriority::HardScript, UObject* Instigator = nullptr);

	bool OnEvent(UActionBrainComponentAction* Action, EPawnActionEventType::Type Event);
	bool PushAction(UActionBrainComponentAction* NewAction, EAIRequestPriority::Type Priority, UObject* Instigator);

	EPawnActionAbortState::Type AbortAction(UActionBrainComponentAction* ActionToAbort);
	EPawnActionAbortState::Type ForceAbortAction(UActionBrainComponentAction* ActionToAbort);

	UFUNCTION(BlueprintCallable, Category = ActionBrainComponent)
	int32 AbortActionsInstigatedBy(UObject* const Instigator, TEnumAsByte<EAIRequestPriority::Type> Priority);

	bool HasActionsForInstigator(UObject* const Instigator) const;

protected:
	UFUNCTION()
	void OnPawnUpdated(ACoreAIController* AIController, ACoreCharacter* Character);

	void RemoveEventsForAction(UActionBrainComponentAction* Action);
	void UpdateCurrentAction();

protected:
	UPROPERTY(EditDefaultsOnly, Category = ActionBrainComponent)
	TSubclassOf<UActionBrainComponentAction> DefaultActionClass = nullptr;

private:
	UPROPERTY(Transient)
	TArray<FActionStack> ActionStacks;

	UPROPERTY(Transient)
	TArray<FActionEvent> ActionEvents;

	UPROPERTY(Transient)
	UActionBrainComponentAction* CurrentAction;

	UPROPERTY()
	bool bIsRunning = false;

	UPROPERTY()
	bool bPaused = false;

	UPROPERTY()
	bool bIteratingActionEvents = false;

	UPROPERTY()
	ACoreCharacter* Pawn = nullptr;

	uint32 ActionEventIndex = 0;
};