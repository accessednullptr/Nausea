// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "AI/RoutineManager/LoopRoutine.h"
#include "LoopRoutineSelectAbilityRoutine.generated.h"

class UAbilityComponent;
class UAbilityInfo;
class UActionBrainDataObject;

USTRUCT(BlueprintType)
struct NAUSEA_API FRoutineAbilityEntry
{
	GENERATED_USTRUCT_BODY()

	FRoutineAbilityEntry() {}

public:
	const TSubclassOf<UAbilityInfo>& GetAbilityClass() const { return AbilityClass; }

	bool IsValid() const;
	bool CanBeSelected(const UAbilityComponent* AbilityComponent) const;

	int32 GetPriority() const { return Priority; }
	float GetWeight() const { return Weight; }

	void SetClass(TSubclassOf<UAbilityInfo> InAbilityClass) {}//AbilityClass = InAbilityClass; }

protected:
	//Priority of this routine action.
	UPROPERTY(EditDefaultsOnly, Category = RoutineAbilityEntry)
	int32 Priority = 1;
	
	//If multiple routine actions have the same priority, one will be randomly selected using a weighted random.
	UPROPERTY(EditDefaultsOnly, Category = RoutineAbilityEntry)
	float Weight = 1.f;

	UPROPERTY()
	TSubclassOf<UAbilityInfo> AbilityClass = nullptr;
};

/**
 * 
 */
UCLASS()
class NAUSEA_API ULoopRoutineSelectAbilityRoutine : public ULoopRoutine
{
	GENERATED_UCLASS_BODY()
	
//~ Begin URoutine Interface
public:
	virtual void StartRoutine() override;
	virtual FString DescribeRoutineToGameplayDebugger() const override;
protected:
	virtual URoutineAction* CreateRoutineAction(TSubclassOf<URoutineAction> RoutineActionClass, bool bAutoStart = true) override;
//~ End URoutine Interface

//~ Begin ULoopRoutine Interface
protected:
	virtual TSubclassOf<URoutineAction> GetNextRoutineAction() override;
//~ End ULoopRoutine Interface

protected:
	FString DescribeAbilityMapToGameplayDebugger() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = Routine)
	TMap<TSubclassOf<UAbilityInfo>, FRoutineAbilityEntry> RoutineAbilityMap;

	UPROPERTY()
	TWeakObjectPtr<UAbilityComponent> CachedAbilityComponent = nullptr;

	UPROPERTY()
	FRoutineAbilityEntry NextSelectedAbility;
};
