// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/EngineTypes.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "Actions/PawnAction.h"
#include "Navigation/PathFollowingComponent.h"
#include "AI/ActionBrainComponentAction.h"
#include "ActionMoveTo.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API UActionMoveTo : public UActionBrainComponentAction
{
	GENERATED_UCLASS_BODY()
	
protected:
	UPROPERTY(BlueprintReadOnly, Category = Action)
	AActor* GoalActor;

	UPROPERTY(BlueprintReadOnly, Category = Action)
	FVector GoalLocation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Move, meta = (ClampMin = "0.01"))
	float AcceptableRadius;

	/** "None" will result in default filter being used */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Move)
	TSubclassOf<UNavigationQueryFilter> FilterClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Move)
	uint32 bAllowStrafe : 1;
	
	/** if set to true (default) will make action succeed when the pawn's collision component overlaps with goal's collision component */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Move)
	uint32 bFinishOnOverlap : 1;

	/** if set, movement will use path finding */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Move)
	uint32 bUsePathfinding : 1;

	/** if set, use incomplete path when goal can't be reached */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Move)
	uint32 bAllowPartialPath : 1;

	/** if set, GoalLocation will be projected on navigation before using  */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Move)
	uint32 bProjectGoalToNavigation : 1;

	/** if set, path to GoalActor will be updated with goal's movement */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Move)
	uint32 bUpdatePathToGoal : 1;

	/** if set, other actions with the same priority will be aborted when path is changed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Move)
	uint32 bAbortChildActionOnPathChange : 1;

public:
	virtual void BeginDestroy() override;

	static bool CheckAlreadyAtGoal(AAIController* Controller, const FVector& TestLocation, float Radius);
	static bool CheckAlreadyAtGoal(AAIController* Controller, const AActor* TestGoal, float Radius);

	virtual void HandleAIMessage(UBrainComponent*, const FAIMessage&) override;

	void SetPath(FNavPathSharedRef InPath);
	virtual void OnPathUpdated(FNavigationPath* UpdatedPath, ENavPathEvent::Type Event);

	void SetAcceptableRadius(float NewAcceptableRadius) { AcceptableRadius = NewAcceptableRadius; }
	void SetFinishOnOverlap(bool bNewFinishOnOverlap) { bFinishOnOverlap = bNewFinishOnOverlap; }
	void EnableStrafing(bool bNewStrafing) { bAllowStrafe = bNewStrafing; }
	void EnablePathUpdateOnMoveGoalLocationChange(bool bEnable) { bUpdatePathToGoal = bEnable; }
	void EnableGoalLocationProjectionToNavigation(bool bEnable) { bProjectGoalToNavigation = bEnable; }
	void EnableChildAbortionOnPathUpdate(bool bEnable) { bAbortChildActionOnPathChange = bEnable; }
	void SetFilterClass(TSubclassOf<UNavigationQueryFilter> NewFilterClass) { FilterClass = NewFilterClass; }
	void SetAllowPartialPath(bool bEnable) { bAllowPartialPath = bEnable; }

	UFUNCTION(BlueprintCallable, Category = Action, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static UActionMoveTo* CreateMoveToAction(const UObject* WorldContextObject, AActor* InGoalActor, FVector InGoalLocation, bool bInUsePathFinding = true);

	virtual class UActionBrainDataObject* GetMoveTargetDataObject() const { return GetDataObject(); }

protected:
	/** currently followed path */
	FNavPathSharedPtr Path;

	FDelegateHandle PathObserverDelegateHandle;
	
	/** Handle for efficient management of DeferredPerformMoveAction timer */
	FTimerHandle TimerHandle_DeferredPerformMoveAction;

	/** Handle for efficient management of TryToRepath timer */
	FTimerHandle TimerHandle_TryToRepath;

	void ClearPath();
	virtual bool Start() override;
	virtual bool Pause(const UActionBrainComponentAction* PausedBy) override;
	virtual bool Resume() override;
	virtual void OnFinished(EPawnActionResult::Type WithResult) override;
	virtual EPawnActionAbortState::Type PerformAbort(EAIForceParam::Type ShouldForce) override;
	virtual bool IsPartialPathAllowed() const;

	virtual EPathFollowingRequestResult::Type RequestMove(AAIController* Controller);
	
	virtual bool PerformMoveAction();

	UFUNCTION()
	void DeferredPerformMoveAction();

	UFUNCTION()
	virtual void OnMoveDataObjectReady(UActionBrainDataObject* DataObject);

	void TryToRepath();
	void ClearPendingRepath();
	void ClearTimers();
};
