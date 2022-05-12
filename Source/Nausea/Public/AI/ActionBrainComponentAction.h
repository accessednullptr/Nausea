// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UObject/ObjectMacros.h"
#include "AIModule/Classes/Actions/PawnAction.h"
#include "ActionStack.h"
#include "ActionBrainComponentAction.generated.h"

class UActionBrainComponent;
class UActionBrainDataObject;
class ACoreCharacter;

NAUSEA_API DECLARE_LOG_CATEGORY_EXTERN(LogActionBrainAction, Warning, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FActionFinishedSignature, UActionBrainComponentAction*, Action, TEnumAsByte<EPawnActionResult::Type>, Result);

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class NAUSEA_API UActionBrainComponentAction : public UObject
{
	GENERATED_UCLASS_BODY()
	
	friend UActionBrainComponent;
	friend FActionStack;
	
//~ Begin UObject Interface
public:
	virtual UWorld* GetWorld() const override;
//~ End UObject Interface

public:
	UFUNCTION(BlueprintCallable, Category = Action, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext, DeterminesOutputType = "ActionClass"))
	static UActionBrainComponentAction* CreateAction(const UObject* WorldContextObject, TSubclassOf<UActionBrainComponentAction> ActionClass);

	UFUNCTION(BlueprintCallable, Category = Action)
	bool SetDataObject(UActionBrainDataObject* DataObject);

	FORCEINLINE const UActionBrainComponentAction* GetParentAction() const { return ParentAction; }
	FORCEINLINE const UActionBrainComponentAction* GetChildAction() const { return ChildAction; }
	FORCEINLINE UActionBrainComponentAction* GetChildAction() { return ChildAction; }

	FORCEINLINE bool WantsTick() const { return bWantsTick; }

	UFUNCTION(BlueprintCallable, Category = Action)
	FORCEINLINE bool IsPaused() const { return !!bPaused; }
	UFUNCTION(BlueprintCallable, Category = Action)
	FORCEINLINE bool IsActive() const { return FinishResult == EPawnActionResult::InProgress && IsPaused() == false && AbortState == EPawnActionAbortState::NotBeingAborted; }
	UFUNCTION(BlueprintCallable, Category = Action)
	FORCEINLINE bool IsBeingAborted() const { return AbortState != EPawnActionAbortState::NotBeingAborted; }
	UFUNCTION(BlueprintCallable, Category = Action)
	FORCEINLINE bool IsFinished() const { return FinishResult > EPawnActionResult::InProgress; }
	UFUNCTION(BlueprintCallable, Category = Action)
	FORCEINLINE bool ShouldPauseMovement() const { return bShouldPauseMovement; }
	UFUNCTION(BlueprintCallable, Category = Action)
	FORCEINLINE bool HasBeenStarted() const { return AbortState != EPawnActionAbortState::NeverStarted; }

	UFUNCTION(BlueprintCallable, Category = Action)
	FString GetStateDescription() const;
	UFUNCTION(BlueprintCallable, Category = Action)
	FString GetPriorityName() const;
	UFUNCTION(BlueprintCallable, Category = Action)
	virtual FString GetDisplayName() const;
	UFUNCTION(BlueprintCallable, Category = Action)
	FString GetDebugInfoString(int32 Depth) const;

	static FString GetNoneActionInfoString(int32 Depth) { return FString::ChrN(Depth * 4, ' ') + FString("(None)\n"); }

	UFUNCTION(BlueprintCallable, Category = Action)
	FORCEINLINE TEnumAsByte<EAIRequestPriority::Type> GetPriority() const { return ExecutionPriority; }
	UFUNCTION(BlueprintCallable, Category = Action)
	FORCEINLINE TEnumAsByte<EPawnActionResult::Type> GetResult() const { return FinishResult; }
	UFUNCTION(BlueprintCallable, Category = Action)
	FORCEINLINE TEnumAsByte<EPawnActionAbortState::Type> GetAbortState() const { return AbortState; }
	UFUNCTION(BlueprintCallable, Category = Action)
	FORCEINLINE UActionBrainComponent* GetOwnerComponent() const { return OwnerComponent; }
	UFUNCTION(BlueprintCallable, Category = Action)
	FORCEINLINE UObject* GetInstigator() const { return Instigator; }
	UFUNCTION(BlueprintCallable, Category = Action)
	FORCEINLINE UActionBrainDataObject* GetDataObject() const { return ActionDataObject; }

	UFUNCTION(BlueprintCallable, Category = Action)
	ACoreCharacter* GetPawn() const;

	UFUNCTION(BlueprintPure, Category = Action)
	AAIController* GetController() const;

	UFUNCTION(BlueprintPure, Category = Action)
	TEnumAsByte<EAIRequestPriority::Type> GetActionPriority() { return ExecutionPriority; }

	template<class TActionClass>
	static TActionClass* CreateActionInstance(UWorld* World)
	{
		TSubclassOf<UActionBrainComponentAction> ActionClass = TActionClass::StaticClass();
		return NewObject<TActionClass>(World, ActionClass);
	}

public:
	UPROPERTY(BlueprintAssignable, Category = Action)
	FActionFinishedSignature OnActionFinished;

protected:
	FORCEINLINE void TickAction(float DeltaTime)
	{
		// tick ChildAction 
		if (ChildAction != NULL)
		{
			ChildAction->Tick(DeltaTime);
		}
		// or self if not paused
		else if (!!bWantsTick && IsPaused() == false)
		{
			Tick(DeltaTime);
		}
	}

	/** triggers aborting of an Action
	 *	@param bForce
	 *	@return current state of task abort
	 *	@NOTE do not make this virtual! Contains some essential logic. */
	EPawnActionAbortState::Type Abort(EAIForceParam::Type ShouldForce = EAIForceParam::DoNotForce);

	/** starts or resumes action, depending on internal state */
	bool Activate();
	void OnPopped();
	void CleanUp();

	UFUNCTION(BlueprintCallable, Category = Action)
	virtual void Finish(TEnumAsByte<EPawnActionResult::Type> WithResult);

	void SendEvent(EPawnActionEventType::Type Event);

	void WaitForMessage(FName MessageType, FAIRequestID InRequestID = FAIRequestID::AnyRequest);
	virtual void HandleAIMessage(UBrainComponent*, const FAIMessage&) {};
	void StopWaitingForMessages();

	void SetOwnerComponent(UActionBrainComponent* Component);

	void SetInstigator(UObject* const InInstigator);

	virtual void Tick(float DeltaTime);

	/** called to start off the Action
	 *	@return 'true' if actions successfully started.
	 *	@NOTE if action fails to start no finishing or aborting mechanics will be triggered
	 *  @NOTE always call super first so that K2_PreStart is called at the correct time. */
	virtual bool Start();
	/** called to pause action when higher priority or child action kicks in */
	virtual bool Pause(const UActionBrainComponentAction* PausedBy);
	/** called to resume action after being paused */
	virtual bool Resume();
	/** called when this action is being removed from action stacks */
	virtual void OnFinished(EPawnActionResult::Type WithResult);
	/** called to give Action chance to react to child action finishing.
	 *	@NOTE gets called _AFTER_ child's OnFinished to give child action chance
	 *		to prepare "finishing data" for parent to read.
	 *	@NOTE clears parent-child binding */
	virtual void OnChildFinished(UActionBrainComponentAction* Action, EPawnActionResult::Type WithResult);

	/** apart from doing regular push request copies additional values from Parent, like Priority and Instigator */
	bool PushChildAction(UActionBrainComponentAction* Action);

	/** performs actual work on aborting Action. Should be called exclusively by Abort function
	 *	@return only valid return values here are LatendAbortInProgress and AbortDone */
	virtual EPawnActionAbortState::Type PerformAbort(EAIForceParam::Type ShouldForce);

	UFUNCTION(BlueprintImplementableEvent, Category = Action, meta = (DisplayName = "PreStart"))
	void K2_PreStart();
	UFUNCTION(BlueprintImplementableEvent, Category = Action, meta = (DisplayName = "Post Start"))
	void K2_PostStart();
	UFUNCTION(BlueprintImplementableEvent, Category = Action, meta = (DisplayName = "On Pause"))
	void K2_Pause();
	UFUNCTION(BlueprintImplementableEvent, Category = Action, meta = (DisplayName = "On Resume"))
	void K2_Resume();
	UFUNCTION(BlueprintImplementableEvent, Category = Action, meta = (DisplayName = "On Finished"))
	void K2_Finished(EPawnActionResult::Type WithResult);


private:
	/** called when this action is put on a stack. Does not indicate action will be started soon
	 *	(it depends on other actions on other action stacks. Called before Start() call */
	void OnPushed();

	/** Sets final result for this Action. To be called only once upon Action's finish */
	void SetFinishResult(EPawnActionResult::Type Result);

	// do not un-private. Internal logic only!
	void SetAbortState(EPawnActionAbortState::Type NewAbortState);

protected:
	/** Current child node executing on top of this Action */
	UPROPERTY(Transient)
	UActionBrainComponentAction* ChildAction;

	UPROPERTY(Transient)
	UActionBrainComponentAction* ParentAction;

	/** Extra reference to the component this action is being governed by */
	UPROPERTY(Transient)
	UActionBrainComponent* OwnerComponent;
	
	/** indicates an object that caused this action. Used for mass removal of actions 
	 *	by specific object */
	UPROPERTY(Transient)
	UObject* Instigator;

	UPROPERTY(Transient)
	FAIRequestID RequestID;

	/** this action's intrinsic execution priority. can be overridden at runtime */
	UPROPERTY(Category = Action, EditDefaultsOnly)
	TEnumAsByte<EAIRequestPriority::Type> ExecutionPriority;

	/** if this is FALSE and we're trying to push a new instance of a given class,
	 *	but the top of the stack is already an instance of that class ignore the attempted push */
	UPROPERTY(Category = Action, EditDefaultsOnly, BlueprintReadOnly)
	uint32 bAllowNewSameClassInstance : 1;

	/** if this is TRUE, when we try to push a new instance of an action who has the
	 *	same class as the action on the top of the stack, pop the one on the stack, and push the new one
	 *	NOTE: This trumps bAllowNewClassInstance (e.g. if this is true and bAllowNewClassInstance
	 *	is false the active instance will still be replaced) */
	UPROPERTY(Category = Action, EditDefaultsOnly, BlueprintReadOnly)
	uint32 bReplaceActiveSameClassInstance : 1;

	/** this is a temporary solution to allow having movement action running in background while there's 
	 *	another action on top doing its thing
	 *	@note should go away once AI resource locking comes on-line */
	UPROPERTY(Category = Action, EditDefaultsOnly, BlueprintReadOnly)
	uint32 bShouldPauseMovement : 1;

	/** if set, action will call OnFinished notify even when ending as FailedToStart */
	UPROPERTY(Category = Action, EditDefaultsOnly, BlueprintReadOnly, AdvancedDisplay)
	uint32 bAlwaysNotifyOnFinished : 1;

private:
	TArray<FAIMessageObserverHandle> MessageHandlers;

	/** indicates whether action is in the process of abortion, and if so on what state */
	EPawnActionAbortState::Type AbortState;
	
	EPawnActionResult::Type FinishResult;
	
	/** Used exclusively for action events sorting */
	int32 IndexOnStack;
	
	/** Indicates the action has been paused */
	uint32 bPaused : 1;

	uint32 bHasBeenStarted : 1;

	/** set to true when action fails the initial Start call */
	uint32 bFailedToStart : 1;

	UPROPERTY(EditDefaultsOnly, Category = Action, Instanced)
	UActionBrainDataObject* ActionDataObject = nullptr;

protected:
	/** TickAction will get called only if this flag is set. To be set in derived action's
	 *	constructor. 
	 *	@NOTE Toggling at runtime is not supported */
	uint32 bWantsTick : 1;
};
