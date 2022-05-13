// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "NauseaCameraComponent.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API UNauseaCameraComponent : public UCameraComponent
{
	GENERATED_UCLASS_BODY()
	
public:
	virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) override;
};
