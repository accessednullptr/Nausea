// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AI/ActionBrainComponentAction.h"
#include "AI/Action/ActionMoveTo.h"
#include "AI/Action/ActionSequence.h"
#include "AI/RoutineManager/Routine.h"
#include "AI/RoutineManager/RoutineActionPushAction.h"
#include "AI/Action/ActionBrainAbilityInterface.h"
#include "Gameplay/Ability/AbilityTypes.h"
#include "ActionPerformAbility.generated.h"

UENUM(BlueprintType)
enum class EPauseResponse : uint8
{
	RestartOnResume,
	Ignore,
	Abort
};

/**
 * 
 */
UCLASS()
class NAUSEA_API UActionPerformAbility : public UActionMoveTo, public IActionBrainAbilityInterface
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UActionBrainComponentAction Interface
protected:
	virtual bool Start() override;
	virtual bool Pause(const UActionBrainComponentAction* PausedBy) override;
	virtual bool Resume() override;
	virtual EPawnActionAbortState::Type PerformAbort(EAIForceParam::Type ShouldForce) override;
	virtual void HandleAIMessage(UBrainComponent*, const FAIMessage& Message) override;
//~ End UActionBrainComponentAction Interface

//~ Begin UActionMoveTo Interface
public:
	virtual UActionBrainDataObject* GetMoveTargetDataObject() const override { return MoveDataObject; }
protected:
	virtual bool PerformMoveAction() override;
	virtual void OnMoveDataObjectReady(UActionBrainDataObject* DataObject) override;
//~ End UActionMoveTo Interface

//~ Begin IActionBrainAbilityInterface Interface	
public:
	virtual void SetAbilityClass(UClass* InAbilityClass) override;
	virtual void SetOverrideCompleteCondition(ECompleteCondition CompleteConditionOverride) override;
//~ End IActionBrainAbilityInterface Interface	

public:
	UFUNCTION()
	bool IsReadyToPerformAbility() const;

protected:
	//Returns true if an ability was successfully started.
	UFUNCTION()
	bool PerformAbility();
	UFUNCTION()
	void AbortAbility();

	UFUNCTION()
	void OnActionDataObjectReady(UActionBrainDataObject* DataObject);
	UFUNCTION()
	void OnLanded(const FHitResult& HitResult);
	UFUNCTION()
	void OnStartDelayExpired();

protected:
	UPROPERTY(EditDefaultsOnly, Category = Action)
	EPauseResponse PauseResponse = EPauseResponse::RestartOnResume;

	UPROPERTY(EditDefaultsOnly, Category = Action, meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bOverrideCompleteCondition = false;
	UPROPERTY(EditDefaultsOnly, Category = Action, meta = (EditCondition = bOverrideCompleteCondition))
	ECompleteCondition OverrideCompleteCondition = ECompleteCondition::CastComplete;

	UPROPERTY(EditDefaultsOnly, Category = Action)
	TSubclassOf<class UAbilityInfo> AbilityClass = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = Action)
	bool bPerformMovement = false;

	UPROPERTY(EditDefaultsOnly, Category = Move, Instanced)
	UActionBrainDataObject* MoveDataObject = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Move)
	bool bAbortMoveOnBlocked = false;

	UPROPERTY(Transient)
	bool bAwaitingDataObject = false;
	UPROPERTY(Transient)
	bool bAwaitingMoveDataObject = false;
	UPROPERTY(Transient)
	bool bAwaitingLanding = false;
	UPROPERTY(Transient)
	bool bAwaitingCastDelay = false;
	UPROPERTY(Transient)
	bool bIsPerformingAbility = false;

	UPROPERTY()
	FAbilityInstanceHandle CurrentAbilityInstanceHandle = FAbilityInstanceHandle();
};

UCLASS()
class NAUSEA_API UActionSequencePerformAbility : public UActionSequence, public IActionBrainAbilityInterface
{
	GENERATED_UCLASS_BODY()

//~ Begin IActionBrainAbilityInterface Interface	
public:
	virtual void SetAbilityClass(UClass* InAbilityClass) override;
	virtual void SetOverrideCompleteCondition(ECompleteCondition CompleteConditionOverride) override;
//~ End IActionBrainAbilityInterface Interface	

protected:
	UPROPERTY()
	TSubclassOf<class UAbilityInfo> AbilityClass = nullptr;
};

class UAbilityInfo;

UCLASS()
class NAUSEA_API URoutineActionPushAbilityAction : public URoutineActionPushAction, public IActionBrainAbilityInterface
{
	GENERATED_UCLASS_BODY()

//~ Begin URoutineActionPushAction Interface
protected:
	virtual UActionBrainComponentAction* CreateAction() override;
//~ End URoutineActionPushAction Interface

//~ Begin IActionBrainAbilityInterface Interface	
public:
	virtual void SetAbilityClass(UClass* InAbilityClass) { AbilityClass = InAbilityClass; }
	virtual void SetOverrideCompleteCondition(ECompleteCondition CompleteConditionOverride) override;
//~ End IActionBrainAbilityInterface Interface	

protected:
	UPROPERTY(EditDefaultsOnly, Category = Ability)
	TSubclassOf<class UAbilityInfo> AbilityClass;

	UPROPERTY(EditDefaultsOnly, Category = Action, meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bOverrideCompleteCondition = false;
	UPROPERTY(EditDefaultsOnly, Category = Action, meta = (EditCondition = bOverrideCompleteCondition))
	ECompleteCondition OverrideCompleteCondition = ECompleteCondition::CastComplete;
};