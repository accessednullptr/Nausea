// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "CoreCameraComponent.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API UCoreCameraComponent : public UCameraComponent
{
	GENERATED_UCLASS_BODY()
	
public:
	virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) override;
};
