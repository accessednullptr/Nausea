// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PilotMovementComponent.generated.h"

UENUM(BlueprintType)
enum ECustomMovementMode
{
	CUSTOMMOVE_None = 0,
	CUSTOMMOVE_Vehicle = 1
};

/**
 * 
 */
UCLASS()
class NAUSEA_API UPilotMovementComponent : public UCharacterMovementComponent
{
	GENERATED_UCLASS_BODY()

//~ Begin UCharacterMovementComponent Interface
protected:
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
//~ End UCharacterMovementComponent Interface

};
