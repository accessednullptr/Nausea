// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "AI/RoutineManager/LoopRoutine.h"
#include "LoopSelectBestRoutine.generated.h"

class URoutineAction;

USTRUCT(BlueprintType)
struct NAUSEA_API FRoutineActionEntry
{
	GENERATED_USTRUCT_BODY()

	FRoutineActionEntry() {}

public:
	const TSubclassOf<URoutineAction>& GetRoutineActionClass() const { return RoutineActionClass; }

	bool IsValid() const { return RoutineActionClass != nullptr; }

	bool NotifyCompleted(UObject* WorldContextObject);

	bool CanBeSelected(const UObject* WorldContextObject) const { return IsValid() && !IsOnCooldown(WorldContextObject) && HasRemainingExecutions(); }

	bool IsOnCooldown(const UObject* WorldContextObject) const;
	float GetCooldownRemaining(const UObject* WorldContextObject) const;

	bool HasRemainingExecutions() const { return ExecutionLimit > 0 ? ExecutionCount < ExecutionLimit : true; }
	int32 GetRemainingExecutions() const { return ExecutionLimit > 0 ? ExecutionLimit - ExecutionCount : -1; }

	int32 GetPriority() const { return Priority; }
	float GetWeight() const { return Weight; }

	void SetClass(TSubclassOf<URoutineAction> InRoutineActionClass) { RoutineActionClass = InRoutineActionClass; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = RoutineActionEntry)
	float CooldownDuration = 3.f;
	UPROPERTY()
	FTimerHandle CooldownTimerHandle;

	//Priority of this routine action.
	UPROPERTY(EditDefaultsOnly, Category = RoutineActionEntry)
	int32 Priority = 1;
	
	//If multiple routine actions have the same priority, one will be randomly selected using a weighted random.
	UPROPERTY(EditDefaultsOnly, Category = RoutineActionEntry)
	float Weight = 1.f;

	UPROPERTY()
	TSubclassOf<URoutineAction> RoutineActionClass = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = RoutineActionEntry)
	int32 ExecutionLimit = 0;
	UPROPERTY()
	int32 ExecutionCount = 0;
};

/**
 * 
 */
UCLASS()
class NAUSEA_API ULoopSelectBestRoutine : public ULoopRoutine
{
	GENERATED_UCLASS_BODY()
		
//~ Begin URoutine Interface
public:
	virtual void StartRoutine() override;
	virtual void RoutineActionCompleted(URoutineAction* RoutineAction) override;
	virtual FString DescribeRoutineToGameplayDebugger() const override;
//~ End URoutine Interface

//~ Begin ULoopRoutine Interface
protected:
	virtual TSubclassOf<URoutineAction> GetNextRoutineAction() override;
//~ End ULoopRoutine Interface

protected:
	FString DescribeActionMapToGameplayDebugger() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = Routine)
	TMap<TSubclassOf<URoutineAction>,FRoutineActionEntry> RoutineActionMap;
};
