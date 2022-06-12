// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UI/CoreWidgetComponent.h"
#include "NauseaWidgetComponent.generated.h"

/**
 * Adds context information so that UCoreUserWidgets are able to generically know if they are WidgetComponent widgets and who their owners are.
 */
UCLASS(Blueprintable, meta=(BlueprintSpawnableComponent))
class NAUSEA_API UNauseaWidgetComponent : public UCoreWidgetComponent
{
	GENERATED_UCLASS_BODY()

//~ Begin UWidgetComponent Interface
public:
	virtual void InitWidget() override;
//~ Begin UWidgetComponent Interface

public:
	bool ShouldHideWidget() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category=UserInterface)
	bool bHideIfOnLocalCharacter = true;
};
