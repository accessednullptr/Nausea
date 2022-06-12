// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DefaultPawn.h"
#include "OverlordPawn.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API AOverlordPawn : public ADefaultPawn
{
	GENERATED_UCLASS_BODY()
	
//~ Begin APawn Interface
protected:
	virtual void SetupPlayerInputComponent(UInputComponent* InInputComponent) override;
//~ End APawn Interface

protected:
	void MouseLookPitch(float Val);
	void MouseLookYaw(float Val);
	void RequestMouseLook();
	void ReleaseMouseLook();

protected:
	UPROPERTY()
	bool bMouseLook = false;
};
