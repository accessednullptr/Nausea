// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetInteractionComponent.h"
#include "CoreWidgetInteractionComponent.generated.h"

class UInputComponent;
class ACorePlayerController;

/**
 * 
 */
UCLASS()
class NAUSEA_API UCoreWidgetInteractionComponent : public UWidgetInteractionComponent
{
	GENERATED_UCLASS_BODY()

//~ Begin UActorComponent Interface
protected:
	virtual void OnRegister() override;
	virtual FWidgetTraceResult PerformTrace() const override;
//~ End UActorComponent Interface

public:
	virtual void SetupInputComponent(UInputComponent* InputComponent);

protected:
	UFUNCTION()
	void MouseButtonPressed();
	UFUNCTION()
	void MouseButtonReleased();

private:
	UPROPERTY(Transient)
	ACorePlayerController* OwningPlayerController = nullptr;
};
