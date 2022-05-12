// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "Player/NauseaCameraComponent.h"
#include "GameFramework/Pawn.h"

UNauseaCameraComponent::UNauseaCameraComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UNauseaCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView)
{
	if (bUsePawnControlRotation)
	{
		const APawn* OwningPawn = Cast<APawn>(GetOwner());

		if(OwningPawn)
		{
			const FRotator PawnViewRotation = OwningPawn->GetViewRotation();
			
			if(!PawnViewRotation.Equals(GetComponentRotation()))
			{
				SetWorldRotation(PawnViewRotation);
			}
		}
	}

	if (bUseAdditiveOffset)
	{
		FTransform OffsetCamToBaseCam = AdditiveOffset;
		FTransform BaseCamToWorld = GetComponentToWorld();
		FTransform OffsetCamToWorld = OffsetCamToBaseCam * BaseCamToWorld;

		DesiredView.Location = OffsetCamToWorld.GetLocation();
		DesiredView.Rotation = OffsetCamToWorld.Rotator();
	}
	else
	{
		DesiredView.Location = GetComponentLocation();
		DesiredView.Rotation = GetComponentRotation();
	}

	DesiredView.FOV = bUseAdditiveOffset ? (FieldOfView + AdditiveFOVOffset) : FieldOfView;
	DesiredView.AspectRatio = AspectRatio;
	DesiredView.bConstrainAspectRatio = bConstrainAspectRatio;
	DesiredView.bUseFieldOfViewForLOD = bUseFieldOfViewForLOD;
	DesiredView.ProjectionMode = ProjectionMode;
	DesiredView.OrthoWidth = OrthoWidth;
	DesiredView.OrthoNearClipPlane = OrthoNearClipPlane;
	DesiredView.OrthoFarClipPlane = OrthoFarClipPlane;

	// See if the CameraActor wants to override the PostProcess settings used.
	DesiredView.PostProcessBlendWeight = PostProcessBlendWeight;
	if (PostProcessBlendWeight > 0.0f)
	{
		DesiredView.PostProcessSettings = PostProcessSettings;
	}
}