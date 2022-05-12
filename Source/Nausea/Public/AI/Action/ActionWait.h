// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "AI/ActionBrainComponentAction.h"
#include "ActionWait.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API UActionWait : public UActionBrainComponentAction
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Action Move To", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static UActionWait* CreateWaitAction(const UObject* WorldContextObject, float InWaitDuration = 5.f, float InWaitVariance = 0.f);

protected:
	virtual bool Start() override;
	virtual bool Pause(const UActionBrainComponentAction* PausedBy) override;
	virtual bool Resume() override;
	virtual EPawnActionAbortState::Type PerformAbort(EAIForceParam::Type ShouldForce) override;

	UFUNCTION()
	void WaitComplete();

protected:
	UPROPERTY(EditDefaultsOnly, Category = Action)
	float WaitDuration = 5.f;
	UPROPERTY(EditDefaultsOnly, Category = Action)
	float WaitVariance = 0.f;
	UPROPERTY()
	FTimerHandle WaitTimerHandle;
};