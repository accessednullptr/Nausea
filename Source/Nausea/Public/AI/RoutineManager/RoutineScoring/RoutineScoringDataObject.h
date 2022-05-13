// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "RoutineScoringDataObject.generated.h"

class AAIController;
class URoutine;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRoutineScoringObjectSelectedSignature, URoutineScoringDataObject*, ScoringDataObject, URoutine*, Routine);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRoutineScoringObjectCompletedSignature, URoutineScoringDataObject*, ScoringDataObject, URoutine*, Routine);

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class NAUSEA_API URoutineScoringDataObject : public UObject
{
	GENERATED_UCLASS_BODY()
	
public:
	void Initialize(AAIController* InOwningController) { OwningController = InOwningController; }

	UFUNCTION()
	float GetScore() const { return 1.f; }

	UFUNCTION()
	void OnRoutineSelected(URoutine* Routine);
	UFUNCTION()
	void OnRoutineCompleted(URoutine* Routine);

public:
	UPROPERTY(BlueprintAssignable, Category = RoutineScoringObject)
	FRoutineScoringObjectSelectedSignature OnRoutineScoringObjectSelected;
	UPROPERTY(BlueprintAssignable, Category = RoutineScoringObject)
	FRoutineScoringObjectCompletedSignature OnRoutineScoringObjectCompleted;

protected:
	UPROPERTY(EditDefaultsOnly, Instanced)
	TArray<UScoreObject*> ScoreObjectList;

	UPROPERTY()
	AAIController* OwningController = nullptr;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class NAUSEA_API UScoreObject : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	int32 Int = 32;
};