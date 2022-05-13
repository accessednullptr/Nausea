// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#include "Player/CameraModifier/ADSCameraModifier.h"
#include "Player/CorePlayerController.h"
#include "Player/CorePlayerCameraManager.h"

UADSCameraModifier::UADSCameraModifier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

bool UADSCameraModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	Super::ModifyCamera(DeltaTime, InOutPOV);

	TArray<TWeakObjectPtr<UObject>> KeyList;
	ZoomMap.GenerateKeyArray(KeyList);

	TArray<TWeakObjectPtr<UObject>> PendingRemovalKeyList;

	float HighestZoomAmount = 1.f;
	for (TWeakObjectPtr<UObject> Key : KeyList)
	{
		FADSRequest& ZoomRequest = ZoomMap[Key];

		if (!ZoomRequest.IsValid())
		{
			PendingRemovalKeyList.Add(Key);
			continue;
		}

		ZoomRequest.TickZoom(DeltaTime);

		if (ZoomRequest.PendingRemoval())
		{
			PendingRemovalKeyList.Add(Key);
			continue;
		}

		const float ZoomAmount = ZoomRequest.GetZoomAmount();

		if (HighestZoomAmount < ZoomAmount)
		{
			HighestZoomAmount = ZoomAmount;
		}
	}

	InOutPOV.FOV /= HighestZoomAmount;
	return false;
}

void UADSCameraModifier::AddADSRequest(const FADSRequest& ZoomRequest)
{
	if (!ZoomRequest.IsValid())
	{
		return;
	}

	UObject* Requester = ZoomRequest.GetRequester();

	if (ZoomMap.Contains(Requester))
	{
		ZoomMap[Requester].UpdateProperties(ZoomRequest);
		ZoomMap[Requester].SetEnabled(true);
		return;
	}

	FADSRequest& ZoomEntry = ZoomMap.Add(Requester);
	ZoomEntry = ZoomRequest;
	ZoomEntry.SetEnabled(true);
}

void UADSCameraModifier::RemoveADSRequest(UObject* Requester, bool bImmediate)
{
	TWeakObjectPtr<UObject> RequestWeakObjectPointer(Requester);

	if (bImmediate)
	{
		ZoomMap.Remove(RequestWeakObjectPointer);
		return;
	}

	if (!ZoomMap.Contains(RequestWeakObjectPointer))
	{
		return;
	}

	ZoomMap[RequestWeakObjectPointer].SetEnabled(false);
}

void UADSCameraModifier::RemoveAllADSRequests(bool bImmediate)
{
	if (bImmediate)
	{
		ZoomMap.Empty();
		return;
	}

	TArray<TWeakObjectPtr<UObject>> KeyList;
	ZoomMap.GenerateKeyArray(KeyList);

	for (TWeakObjectPtr<UObject> Key : KeyList)
	{
		ZoomMap[Key].SetEnabled(false);
	}
}

void UADSCameraModifier::CreateZoomRequest(ACorePlayerController* PlayerController, UObject* Requester, float ZoomAmount, float ZoomRate, float ZoomRateWithoutRequester)
{
	if (!PlayerController || !PlayerController->GetPlayerCameraManager() || !PlayerController->GetPlayerCameraManager()->GetADSCameraModifier())
	{
		return;
	}

	FADSRequest Request = FADSRequest(Requester, ZoomAmount, ZoomRate, ZoomRateWithoutRequester != -1.f ? ZoomRateWithoutRequester : -MAX_FLT);
	PlayerController->GetPlayerCameraManager()->GetADSCameraModifier()->AddADSRequest(Request);
}

void UADSCameraModifier::ClearADSRequest(ACorePlayerController* PlayerController, UObject* Requester, bool bImmediate)
{
	if (!PlayerController || !PlayerController->GetPlayerCameraManager() || !PlayerController->GetPlayerCameraManager()->GetADSCameraModifier())
	{
		return;
	}

	PlayerController->GetPlayerCameraManager()->GetADSCameraModifier()->RemoveADSRequest(Requester, bImmediate);
}

void UADSCameraModifier::ClearAllRequests(ACorePlayerController* PlayerController, bool bImmediate)
{
	if (!PlayerController || !PlayerController->GetPlayerCameraManager() || !PlayerController->GetPlayerCameraManager()->GetADSCameraModifier())
	{
		return;
	}

	PlayerController->GetPlayerCameraManager()->GetADSCameraModifier()->RemoveAllADSRequests(bImmediate);
}