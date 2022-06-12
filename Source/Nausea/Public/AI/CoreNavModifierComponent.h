// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "NavModifierComponent.h"
#include "CoreNavModifierComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = (Navigation), meta = (BlueprintSpawnableComponent))
class NAUSEA_API UCoreNavModifierComponent : public UNavModifierComponent
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UNavRelevantComponent Interface
public:
	virtual void CalcAndCacheBounds() const override;
//~ End UNavRelevantComponent Interface

};
