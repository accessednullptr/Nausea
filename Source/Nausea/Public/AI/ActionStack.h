#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "ActionStack.generated.h"

class UActionBrainComponentAction;

USTRUCT()
struct NAUSEA_API FActionStack
{
	GENERATED_USTRUCT_BODY()

	FActionStack()
		: TopAction(nullptr)
	{}

private:
	UPROPERTY()
	UActionBrainComponentAction* TopAction = nullptr;

public:
	void Pause();
	void Resume();

	/** All it does is tie actions into a double-linked list making NewTopAction
	 *	new stack's top */
	void PushAction(UActionBrainComponentAction* NewTopAction);

	/** Looks through the double-linked action list looking for specified action
	 *	and if found action will be popped along with all it's siblings */
	void PopAction(UActionBrainComponentAction* ActionToPop);
	
	FORCEINLINE UActionBrainComponentAction* GetTop() const { return TopAction; }

	FORCEINLINE bool IsEmpty() const { return TopAction == NULL; }

	//----------------------------------------------------------------------//
	// Debugging-testing purposes 
	//----------------------------------------------------------------------//
	int32 GetStackSize() const;
};
