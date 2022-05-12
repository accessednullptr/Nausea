#pragma once

#include "CoreMinimal.h"
#include "InteractableTypes.generated.h"

UENUM(BlueprintType)
enum class EInteractionResponse : uint8
{
	Success,

	//Held interaction.
	Pending,

	//Anything below Failed is a failure state.
	Failed,
	Invalid
};