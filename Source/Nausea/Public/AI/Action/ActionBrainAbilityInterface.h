// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ActionBrainAbilityInterface.generated.h"

enum class ECompleteCondition : uint8;

UINTERFACE(MinimalAPI)
class UActionBrainAbilityInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class NAUSEA_API IActionBrainAbilityInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	virtual void SetAbilityClass(UClass* InAbilityClass) {}
	virtual void SetOverrideCompleteCondition(ECompleteCondition CompleteConditionOverride) {}
};