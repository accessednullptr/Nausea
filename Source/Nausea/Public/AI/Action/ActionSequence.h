// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "AI/ActionBrainComponentAction.h"
#include "ActionSequence.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API UActionSequence : public UActionBrainComponentAction
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = Action)
	TArray<UActionBrainComponentAction*> ActionSequence;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Action)
	TEnumAsByte<EPawnActionFailHandling::Type> ChildFailureHandlingMode;

	UPROPERTY(Transient)
	UActionBrainComponentAction* RecentActionCopy;

	uint32 CurrentActionIndex;

	EPawnSubActionTriggeringPolicy::Type SubActionTriggeringPolicy;

	UFUNCTION(BlueprintCallable, Category = Action, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static UActionSequence* CreateSequenceAction(const UObject* WorldContextObject, TArray<UActionBrainComponentAction*> Actions,
		TEnumAsByte<EPawnActionFailHandling::Type> FailureHandlingMode = EPawnActionFailHandling::RequireSuccess);
	
//~ Begin UActionBrainComponentAction Interface
protected:
	virtual bool Start() override;
	virtual bool Resume() override;
	virtual void OnChildFinished(UActionBrainComponentAction* Action, EPawnActionResult::Type WithResult) override;
	virtual void Finish(TEnumAsByte<EPawnActionResult::Type> WithResult) override;
	virtual FString GetDebugInfoString(int32 Depth) const;
//~ End UActionBrainComponentAction Interface

	bool PushNextActionCopy();
};
