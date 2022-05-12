// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Objective/Objective.h"
#include "ObjectiveDestroyActor.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API UObjectiveDestroyActor : public UObjective
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UObjective Interface
public:
	virtual void SetObjectiveState(EObjectiveState State) override;
	virtual FString DescribeObjectiveToGameplayDebugger() const override;
//~ End UObjective Interface

public:
	UFUNCTION(BlueprintPure, Category = Objective, meta = (WorldContext = "WorldContextObject", DeterminesOutputType = "ObjectiveClass"))
	static UObjectiveDestroyActor* CreateDestroyActorObjective(UObject* WorldContextObject, TSubclassOf<UObjectiveDestroyActor> ObjectiveClass, const FString& InObjectiveID, TArray<AActor*> InActors);

	template<class T>
	static T* CreateDestroyActorObjective(UObject* WorldContextObject, TSubclassOf<UObjectiveDestroyActor> ObjectiveClass, const FString& InObjectiveID, TArray<AActor*> InOverlapActors)
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UObjectiveDestroyActor>::Value, "'T' template parameter to CreateDestroyActorObjective must be derived from UObjectiveDestroyActor");
		return Cast<T>(CreateDestroyActorObjective(WorldContextObject, ObjectiveClass, InObjectiveID, InOverlapActors));
	}

protected:
	UFUNCTION()
	void BindObjectiveEvents();
	UFUNCTION()
	void UnBindObjectiveEvents();

	UFUNCTION()
	void CharacterDestroyed(AActor* Character);
	UFUNCTION()
	void OnActorDied(UStatusComponent* Component, float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION()
	void UpdateObjectiveProgress();

protected:
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> ActorList;
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> RemainingActorList;

	UPROPERTY()
	int32 StartingActorCount = -1;
};
