// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "Player/NauseaPlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "WorldCollision.h"
#include "Engine/World.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Player/NauseaPlayerController.h"
#include "Player/CameraModifier/ADSCameraModifier.h"
#include "Player/CameraModifier/RecoilCameraModifier.h"

ANauseaPlayerCameraManager::ANauseaPlayerCameraManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CameraStyle = CameraStyleName::NAME_Default;

	DefaultModifiers.Add(UADSCameraModifier::StaticClass());
	DefaultModifiers.Add(URecoilCameraModifier::StaticClass());
}

ANauseaPlayerController* ANauseaPlayerCameraManager::GetNauseaPlayerController() const
{
	return Cast<ANauseaPlayerController>(PCOwner);
}