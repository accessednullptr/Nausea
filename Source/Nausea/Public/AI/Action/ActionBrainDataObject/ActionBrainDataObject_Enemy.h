// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AI/Action/ActionBrainDataObject.h"
#include "ActionBrainDataObject_Enemy.generated.h"

class UEnemySelectionComponent;

/**
 * 
 */
UCLASS()
class NAUSEA_API UActionBrainDataObject_Enemy : public UActionBrainDataObject
{
	GENERATED_UCLASS_BODY()

//~ Begin UActionBrainDataObject Interface
public:
	virtual void Initialize(AAIController* InOwningController) override;
	virtual void GetListOfActors(TArray<AActor*>& ActorList) const override;
	virtual void GetListOfLocations(TArray<FVector>& LocationList) const override;
//~ Begin UActionBrainDataObject Interface

protected:
	UPROPERTY(Transient)
	UEnemySelectionComponent* OwningEnemySelectionComponent = nullptr;
};

UCLASS()
class NAUSEA_API UActionBrainDataObject_AllEnemies : public UActionBrainDataObject
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UActionBrainDataObject Interface
public:
	virtual void Initialize(AAIController* InOwningController) override;
	virtual void GetListOfActors(TArray<AActor*>& ActorList) const override;
	virtual void GetListOfLocations(TArray<FVector>& LocationList) const override;
//~ Begin UActionBrainDataObject Interface
	
protected:
	UPROPERTY(Transient)
	UEnemySelectionComponent* OwningEnemySelectionComponent = nullptr;
};