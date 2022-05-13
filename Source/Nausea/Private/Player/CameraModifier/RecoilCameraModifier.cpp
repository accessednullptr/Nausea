// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Player/CameraModifier/RecoilCameraModifier.h"
#include "Player/CorePlayerController.h"
#include "Player/CorePlayerCameraManager.h"

URecoilCameraModifier::URecoilCameraModifier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

bool URecoilCameraModifier::ProcessViewRotation(AActor* ViewTarget, float DeltaTime, FRotator& OutViewRotation, FRotator& OutDeltaRot)
{
	Super::ProcessViewRotation(ViewTarget, DeltaTime, OutViewRotation, OutDeltaRot);

	FVector2D AppliedRecoil;
	for (int32 Index = RecoilList.Num() - 1; Index >= 0; Index--)
	{
		FRecoilRequest& Recoil = RecoilList[Index];
		Recoil.TickRecoil(DeltaTime, AppliedRecoil);
		OutDeltaRot.Add(AppliedRecoil.Y, AppliedRecoil.X, 0.f);

		if (Recoil.PendingRemoval())
		{
			RecoilList.RemoveAtSwap(Index);
		}
	}

	return false;
}

void URecoilCameraModifier::AddRecoilRequest(const FRecoilRequest& RecoilRequest)
{
	RecoilList.Add(RecoilRequest);
}

void URecoilCameraModifier::RemoveAllRecoilRequests()
{
	RecoilList.Empty();
}

void URecoilCameraModifier::CreateRecoilRequest(ACorePlayerController* PlayerController, const FVector2D& Recoil, float RecoilDuration)
{
	if (!PlayerController || !PlayerController->GetPlayerCameraManager() || !PlayerController->GetPlayerCameraManager()->GetRecoilCameraModifier())
	{
		return;
	}

	FRecoilRequest Request = FRecoilRequest(Recoil, 1.f / RecoilDuration);
	PlayerController->GetPlayerCameraManager()->GetRecoilCameraModifier()->AddRecoilRequest(Request);
}

void URecoilCameraModifier::ClearAllRecoilRequests(ACorePlayerController* PlayerController)
{
	if (!PlayerController || !PlayerController->GetPlayerCameraManager() || !PlayerController->GetPlayerCameraManager()->GetRecoilCameraModifier())
	{
		return;
	}

	PlayerController->GetPlayerCameraManager()->GetRecoilCameraModifier()->RemoveAllRecoilRequests();
}