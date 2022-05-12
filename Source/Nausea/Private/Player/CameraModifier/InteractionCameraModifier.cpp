// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "Player/CameraModifier/InteractionCameraModifier.h"
#include "Player/CorePlayerController.h"
#include "Player/CorePlayerCameraManager.h"

UInteractionCameraModifier::UInteractionCameraModifier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bDisabled = true;
}

void UInteractionCameraModifier::EnableModifier()
{
	Super::EnableModifier();
	
	if (CameraOwner)
	{
		OriginalRotation = CameraOwner->ViewTarget.POV.Rotation.GetNormalized();
	}
}

bool UInteractionCameraModifier::ProcessViewRotation(AActor* ViewTarget, float DeltaTime, FRotator& OutViewRotation, FRotator& OutDeltaRot)
{
	FTransform OriginalRotationTransform = FTransform(OriginalRotation, FVector(0));
	FRotator RelativeRotation = OriginalRotationTransform.InverseTransformRotation(OutViewRotation.Quaternion()).Rotator().GetNormalized();

	RelativeRotation.Yaw = FMath::Clamp(RelativeRotation.Yaw, -YawLock, YawLock);
	RelativeRotation.Pitch = FMath::Clamp(RelativeRotation.Pitch, -PitchLock, PitchLock);

	if (RelativeRotation.GetManhattanDistance(FRotator::ZeroRotator) < (RelativeRotation + OutDeltaRot).GetManhattanDistance(FRotator::ZeroRotator))
	{
		OutDeltaRot *= 0.5f;
	}

	const float InterpolationStrength = FMath::Lerp(MagnetismStrength.X, MagnetismStrength.Y, RelativeRotation.GetManhattanDistance(FRotator::ZeroRotator) / ((PitchLock + YawLock) / 2.f));
	if (InterpolationStrength > 0.f)
	{
		RelativeRotation = FMath::RInterpTo(RelativeRotation, FRotator::ZeroRotator, DeltaTime, InterpolationStrength);
	}

	OutViewRotation = OriginalRotationTransform.TransformRotation(RelativeRotation.Quaternion()).Rotator();
	return false;
}

void UInteractionCameraModifier::RequestInteraction(UObject* Requester)
{
	if (!Requester)
	{
		return;
	}

	RequestSet.Add(Requester);
	UpdateInteractionModifier();
}

void UInteractionCameraModifier::ClearInteraction(UObject* Requester)
{
	RequestSet.Remove(Requester);
	UpdateInteractionModifier();
}

void UInteractionCameraModifier::ClearAllInteractions()
{
	RequestSet.Empty();
	UpdateInteractionModifier();
}

void UInteractionCameraModifier::CreateInteractionRequest(ACorePlayerController* PlayerController, UObject* Requester)
{
	if (!PlayerController || !PlayerController->GetPlayerCameraManager() || !PlayerController->GetPlayerCameraManager()->GetInteractionCameraModifier())
	{
		return;
	}

	PlayerController->GetPlayerCameraManager()->GetInteractionCameraModifier()->RequestInteraction(Requester);
}

void UInteractionCameraModifier::ClearInteractionRequest(ACorePlayerController* PlayerController, UObject* Requester)
{
	if (!PlayerController || !PlayerController->GetPlayerCameraManager() || !PlayerController->GetPlayerCameraManager()->GetInteractionCameraModifier())
	{
		return;
	}

	PlayerController->GetPlayerCameraManager()->GetInteractionCameraModifier()->ClearInteraction(Requester);
}

void UInteractionCameraModifier::ClearAllRequests(ACorePlayerController* PlayerController)
{
	if (!PlayerController || !PlayerController->GetPlayerCameraManager() || !PlayerController->GetPlayerCameraManager()->GetInteractionCameraModifier())
	{
		return;
	}

	PlayerController->GetPlayerCameraManager()->GetInteractionCameraModifier()->ClearAllInteractions();
}

void UInteractionCameraModifier::UpdateInteractionModifier()
{
	TArray<TObjectKey<UObject>> RequestArray = RequestSet.Array();
	for (const TObjectKey<UObject>& Request : RequestArray)
	{
		if (!Request.ResolveObjectPtr())
		{
			RequestSet.Remove(Request);
		}
	}

	if (RequestSet.Num() > 0)
	{
		EnableModifier();
	}
	else
	{
		DisableModifier(false);
	}
}