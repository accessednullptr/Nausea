// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "OverlordPawnMovement.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API UOverlordPawnMovement : public UFloatingPawnMovement
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UFloatingPawnMovement Interface
public:
	virtual void AddInputVector(FVector WorldVector, bool bForce = false) override;
	virtual void ApplyControlInputToVelocity(float DeltaTime) override;
//~ End UFloatingPawnMovement Interface
};
