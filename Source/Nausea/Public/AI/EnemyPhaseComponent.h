// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyPhaseComponent.generated.h"

/*
* Convenience component. Has a bunch of accessors and events to help management of an enemy's phases.
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class NAUSEA_API UEnemyPhaseComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()
		
//~ Begin UActorComponent Interface 
protected:
	virtual void BeginPlay() override;		
//~ End UActorComponent Interface 

public:
	UFUNCTION(BlueprintCallable, Category = EnemyPhaseComponent)
	int32 GetCurrentPhase() const { return CurrentPhase; }

protected:
	UFUNCTION(BlueprintCallable, Category = EnemyPhaseComponent, BlueprintAuthorityOnly)
	void SetPhase(int32 NewPhase);

	UFUNCTION(BlueprintImplementableEvent, Category = EnemyPhaseComponent, BlueprintAuthorityOnly)
	void OnPhaseSet(int32 NewPhase, int32 PreviousPhase);

	UFUNCTION()
	void OnRep_CurrentPhase(int32 PreviousPhase);

	UFUNCTION(BlueprintImplementableEvent, Category = EnemyPhaseComponent, BlueprintCosmetic)
	void OnPhaseUpdate(int32 NewPhase, int32 PreviousPhase);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPhase)
	int32 CurrentPhase = INDEX_NONE;

	//If true, SetPhase will be ignored if the new phase is before the current one (a smaller number).
	UPROPERTY(EditDefaultsOnly, Category = EnemyPhaseComponent)
	bool bIgnoreSetToPreviousPhase = true;
};
