#pragma once

#include "CoreMinimal.h"
#include "TrapTypes.generated.h"

const static float TrapGridSize = 100.f;

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EPlacementType : uint8
{
	None = 0 UMETA(Hidden),
	Floor = 1 << 1,
	Wall = 1 << 2,
	Ceiling = 1 << 3
};
ENUM_CLASS_FLAGS(EPlacementType);