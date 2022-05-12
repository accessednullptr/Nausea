#pragma once

#include "CoreMinimal.h"
#include "VehicleTypes.generated.h"

UENUM(BlueprintType)
enum class EVehicleExitType : uint8
{
	Request, //This exit call is coming from a player request.
	Response, //This exit call is coming from a response to a player request.
	Force
};

UENUM(BlueprintType)
enum class EVehicleEnterResponse : uint8
{
	Success,
	Failed,
	Occupied,
};