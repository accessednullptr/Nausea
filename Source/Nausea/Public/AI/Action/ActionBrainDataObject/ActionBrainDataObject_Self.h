// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AI/Action/ActionBrainDataObject.h"
#include "ActionBrainDataObject_Self.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API UActionBrainDataObject_Self : public UActionBrainDataObject
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UActionBrainDataObject Interface
public:
	virtual void Initialize(AAIController* InOwningController) override;
	virtual void GetListOfActors(TArray<AActor*>& ActorList) const override;
	virtual void GetListOfLocations(TArray<FVector>& LocationList) const override;
//~ Begin UActionBrainDataObject Interface
};
