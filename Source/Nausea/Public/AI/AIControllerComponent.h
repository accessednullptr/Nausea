// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AIControllerComponent.generated.h"

class ACoreAIController;
class ACoreCharacter;

UCLASS(ClassGroup=(Custom))
class NAUSEA_API UAIControllerComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

//~ Begin UActorComponent Interface
public:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
//~ End UActorComponent Interface

public:
	UFUNCTION(BlueprintCallable, Category = AIControllerComponent)
	ACoreAIController* GetAIController() const { return OwningAIController; }

protected:
	UFUNCTION()
	virtual void OnPawnUpdated(ACoreAIController* AIController, ACoreCharacter* InCharacter) { K2_OnPawnUpdated (AIController, InCharacter); }
	UFUNCTION(BlueprintImplementableEvent, Category = AIComponent, meta = (DisplayName="On Pawn Updated", ScriptName="OnPawnUpdate"))
	void K2_OnPawnUpdated(ACoreAIController* AIController, ACoreCharacter* InCharacter);

	UFUNCTION(BlueprintImplementableEvent, Category = AIComponent, meta = (DisplayName="On Initialized", ScriptName="OnInitialized"))
	void K2_OnInitialized(ACoreAIController* AIController);

private:
	UPROPERTY(Transient)
	ACoreAIController* OwningAIController = nullptr;
};
