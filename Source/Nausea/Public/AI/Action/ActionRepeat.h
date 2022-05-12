// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AI/ActionBrainComponentAction.h"
#include "ActionRepeat.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API UActionRepeat : public UActionBrainComponentAction
{
	GENERATED_UCLASS_BODY()
	
	enum
	{
		LoopForever = -1
	};

	/** Action to repeat. This instance won't really be run, it's a source for copying actions to be actually performed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = Action)
	UActionBrainComponentAction* ActionToRepeat;

	UPROPERTY(Transient)
	UActionBrainComponentAction* RecentActionCopy;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Action)
	TEnumAsByte<EPawnActionFailHandling::Type> ChildFailureHandlingMode;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Action)
	int32 RepeatCount = LoopForever;

	EPawnSubActionTriggeringPolicy::Type SubActionTriggeringPolicy;

public:
	UFUNCTION(BlueprintCallable, Category = Action, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static UActionRepeat* CreateRepeatAction(const UObject* WorldContextObject, UActionBrainComponentAction* Action, int32 NumberOfRepeats = -1,
		TEnumAsByte<EPawnActionFailHandling::Type> FailureHandlingMode = EPawnActionFailHandling::RequireSuccess);

//~ Begin UActionBrainComponentAction Interface
protected:
	virtual bool Start() override;
	virtual bool Resume() override;
	virtual void OnChildFinished(UActionBrainComponentAction* Action, EPawnActionResult::Type WithResult) override;
	virtual void Finish(TEnumAsByte<EPawnActionResult::Type> WithResult) override;
	virtual FString GetDebugInfoString(int32 Depth) const;
//~ End UActionBrainComponentAction Interface

	bool PushSubAction();
};
