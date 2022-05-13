// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#include "Player/CorePlayerCameraManager.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Player/CorePlayerController.h"

namespace CameraStyleName
{
	const FName NAME_Default = FName(TEXT("Default"));
	const FName NAME_Fixed = FName(TEXT("Fixed"));
	const FName NAME_ThirdPerson = FName(TEXT("ThirdPerson"));
	const FName NAME_FreeCam = FName(TEXT("FreeCam"));
	const FName NAME_FreeCam_Default = FName(TEXT("FreeCam_Default"));
	const FName NAME_FirstPerson = FName(TEXT("FirstPerson"));
}

ACorePlayerCameraManager::ACorePlayerCameraManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

ACorePlayerState* ACorePlayerCameraManager::GetOwningPlayerState() const
{
	return GetPlayerController() ? GetPlayerController()->GetOwningPlayerState() : nullptr;
}

void ACorePlayerCameraManager::InitializeFor(class APlayerController* InPlayerController)
{
	Super::InitializeFor(InPlayerController);

	//Cache all persistent animation modifiers.
	for (UCameraModifier* Modifier : ModifierList)
	{
		if (!Modifier)
		{
			return;
		}

		if (UADSCameraModifier* ADSModifier = Cast<UADSCameraModifier>(Modifier))
		{
			ADSCameraModifier = ADSModifier;
		}
		else if (URecoilCameraModifier* RecoilModifier = Cast<URecoilCameraModifier>(Modifier))
		{
			RecoilCameraModifier = RecoilModifier;
		}
		else if (UInteractionCameraModifier* InteractableModifer = Cast<UInteractionCameraModifier>(Modifier))
		{
			InteractionCameraModifier = InteractableModifer;
		}
	}

	if (ACorePlayerController* CorePlayerController = GetPlayerController())
	{
		CorePlayerController->OnPawnUpdated.AddDynamic(this, &ACorePlayerCameraManager::PawnPossesed);
		CorePlayerController->OnPlayerPawnKilled.AddDynamic(this, &ACorePlayerCameraManager::PossessedPawnKilled);
	}

	OnInitializeCameraManager(GetPlayerController());
}

void ACorePlayerCameraManager::SetViewTarget(AActor* NewViewTarget, FViewTargetTransitionParams TransitionParams)
{
	if (!NewViewTarget)
	{
		NewViewTarget = PCOwner;
	}

	if (NewViewTarget == PCOwner && PCOwner->GetPawnOrSpectator())
	{
		NewViewTarget = PCOwner->GetPawnOrSpectator();
	}

	Super::SetViewTarget(NewViewTarget, TransitionParams);
}

void ACorePlayerCameraManager::AssignViewTarget(AActor* NewTarget, FTViewTarget& VT, FViewTargetTransitionParams TransitionParams)
{
	if (!NewTarget || (NewTarget == VT.Target))
	{
		return;
	}

	//Update camera mode based on the new view target before BecomeViewTarget is called so it can know what's going on.
	if (PCOwner->GetPawnOrSpectator() && PCOwner->GetPawnOrSpectator() == NewTarget)
	{
		PCOwner->SetCameraMode(CameraStyleName::NAME_FirstPerson);
	}
	else
	{
		PCOwner->SetCameraMode(CameraStyleName::NAME_ThirdPerson);
	}

	Super::AssignViewTarget(NewTarget, VT, TransitionParams);
}

void ACorePlayerCameraManager::UpdateViewTarget(FTViewTarget& OutViewTarget, float DeltaTime)
{
	if ((PendingViewTarget.Target != NULL) && BlendParams.bLockOutgoing && OutViewTarget.Equal(ViewTarget))
	{
		return;
	}

	FMinimalViewInfo OrigPOV = OutViewTarget.POV;

	OutViewTarget.POV.FOV = DefaultFOV;
	OutViewTarget.POV.OrthoWidth = DefaultOrthoWidth;
	OutViewTarget.POV.AspectRatio = DefaultAspectRatio;
	OutViewTarget.POV.bConstrainAspectRatio = bDefaultConstrainAspectRatio;
	OutViewTarget.POV.bUseFieldOfViewForLOD = true;
	OutViewTarget.POV.ProjectionMode = bIsOrthographic ? ECameraProjectionMode::Orthographic : ECameraProjectionMode::Perspective;
	OutViewTarget.POV.PostProcessSettings.SetBaseValues();
	OutViewTarget.POV.PostProcessBlendWeight = 1.0f;


	if (ACameraActor* CamActor = Cast<ACameraActor>(OutViewTarget.Target))
	{
		CamActor->GetCameraComponent()->GetCameraView(DeltaTime, OutViewTarget.POV);
		PostUpdateViewTarget(OutViewTarget, true, DeltaTime);
		return;
	}

	bool bDoNotApplyModifiers = false;

	if (CameraStyle == CameraStyleName::NAME_Fixed)
	{
		OutViewTarget.POV = OrigPOV;
		bDoNotApplyModifiers = true;
	}
	else if (CameraStyle == CameraStyleName::NAME_FreeCam || CameraStyle == CameraStyleName::NAME_FreeCam_Default)
	{
		OutViewTarget.POV.Location = OutViewTarget.Target->GetActorLocation();
		OutViewTarget.POV.Rotation = PCOwner->GetControlRotation();
		bDoNotApplyModifiers = true;
	}
	else /*if (CameraStyle == CameraStyleName::NAME_FirstPerson || CameraStyle == CameraStyleName::NAME_ThirdPerson)*/
	{
		UpdateViewTargetInternal(OutViewTarget, DeltaTime);
		/*OutViewTarget.Target->GetActorEyesViewPoint(OutViewTarget.POV.Location, OutViewTarget.POV.Rotation);*/
	}

	PostUpdateViewTarget(OutViewTarget, !bDoNotApplyModifiers, DeltaTime);
}

ACorePlayerController* ACorePlayerCameraManager::GetPlayerController() const
{
	return Cast<ACorePlayerController>(PCOwner);
}

void ACorePlayerCameraManager::SetCameraStyle(const FName& InCameraStyle, bool bUpdateViewTarget)
{
	CameraStyle = InCameraStyle;

	AActor* CurrentViewTarget = ViewTarget.Target;

	//This is our way of notifying the view target of a camera change.
	if (bUpdateViewTarget && CurrentViewTarget && !CurrentViewTarget->IsPendingKillPending())
	{
		CurrentViewTarget->EndViewTarget(PCOwner);

		if (CurrentViewTarget && !CurrentViewTarget->IsPendingKillPending())
		{
			CurrentViewTarget->BecomeViewTarget(PCOwner);
		}
	}
}

void ACorePlayerCameraManager::PawnPossesed(ACorePlayerController* PlayerController, ACoreCharacter* Pawn)
{
	OnPawnPossessed(Pawn);
}

void ACorePlayerCameraManager::PossessedPawnKilled(ACoreCharacter* KilledCharacter, TSubclassOf<UDamageType> DamageType, float Damage, APlayerState* KillerPlayerState)
{
	OnPossessedPawnKilled(KilledCharacter, DamageType, Damage, KillerPlayerState);
}

void ACorePlayerCameraManager::SetCameraMode(ECameraMode CameraMode)
{
	switch (CameraMode)
	{
	case ECameraMode::FirstPerson:
		SetCameraStyle(CameraStyleName::NAME_FirstPerson, true);
		break;
	case ECameraMode::ThirdPerson:
		SetCameraStyle(CameraStyleName::NAME_ThirdPerson, true);
		break;
	case ECameraMode::FreeCamera:
		SetCameraStyle(CameraStyleName::NAME_FreeCam, true);
		break;
	}
}

void ACorePlayerCameraManager::PostUpdateViewTarget(FTViewTarget& OutViewTarget, bool bApplyModifiers, float DeltaTime)
{
	if (bApplyModifiers || bAlwaysApplyModifiers)
	{
		ApplyCameraModifiers(DeltaTime, OutViewTarget.POV);
	}

	SetActorLocationAndRotation(OutViewTarget.POV.Location, OutViewTarget.POV.Rotation, false);
	UpdateCameraLensEffects(OutViewTarget);
}