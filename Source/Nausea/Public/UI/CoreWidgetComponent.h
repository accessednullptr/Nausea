// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "CoreWidgetComponent.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API UCoreWidgetComponent : public UWidgetComponent
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UWidgetComponent Interface
public:
	virtual void InitWidget() override;
//~ Begin UWidgetComponent Interface
};
