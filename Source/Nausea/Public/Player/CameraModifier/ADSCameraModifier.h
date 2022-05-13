// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "ADSCameraModifier.generated.h"

class ACorePlayerController;

USTRUCT(BlueprintType)
struct NAUSEA_API FADSRequest
{
	GENERATED_USTRUCT_BODY()

	FADSRequest(){}

	FADSRequest(UObject* InRequester, float InZoomAmount, float InZoomRate, float InZoomRateWithoutRequester)
	{
		Requester = InRequester;
		ZoomAmount = InZoomAmount;
		ZoomRate = InZoomRate;
		ZoomRateWithoutRequester = InZoomRateWithoutRequester;
	}

public:
	void TickZoom(float DeltaTime)
	{
		if (!Requester.IsValid() && ZoomRateWithoutRequester != -MAX_FLT)
		{
			ZoomRate = ZoomRateWithoutRequester;
		}

		if (IsEnabled())
		{
			ZoomProgress = FMath::Clamp(ZoomProgress + (DeltaTime * ZoomRate), 0.f, 1.f);
		}
		else
		{
			ZoomProgress = FMath::Clamp(ZoomProgress - (DeltaTime * ZoomRate), 0.f, 1.f);
		}
	}

	void SetEnabled(bool bInEnabled) { bEnabled = bInEnabled; }

	void UpdateProperties(const FADSRequest& InProperties)
	{
		ZoomAmount = InProperties.ZoomAmount;
		ZoomRate = InProperties.ZoomRate;
	}

	float GetZoomAmount() const { return FMath::Lerp(1.f, ZoomAmount, ZoomProgress); }
	UObject* GetRequester() const { return Requester.Get(); }
	
	bool IsValid() const { return ZoomAmount != -MAX_FLT && ZoomRate != -MAX_FLT; }
	bool PendingRemoval() const { return ZoomProgress <= 0.f; }

	bool IsEnabled() const { return bEnabled; }

protected:
	UPROPERTY()
	float ZoomAmount = -MAX_FLT;
	UPROPERTY()
	float ZoomRate = -MAX_FLT;
	UPROPERTY()
	float ZoomProgress = 0.f;
	UPROPERTY()
	TWeakObjectPtr<UObject> Requester = nullptr;
	//Zoom rate used when requester is nullptr.
	UPROPERTY()
	float ZoomRateWithoutRequester = -MAX_FLT;
	UPROPERTY()
	bool bEnabled = false;
};


/**
 * 
 */
UCLASS()
class NAUSEA_API UADSCameraModifier : public UCameraModifier
{
	GENERATED_UCLASS_BODY()
	
public:
	virtual bool ModifyCamera(float DeltaTime, struct FMinimalViewInfo& InOutPOV) override;

	UFUNCTION()
	virtual void AddADSRequest(const FADSRequest& ZoomRequest);
	UFUNCTION()
	virtual void RemoveADSRequest(UObject* Requester, bool bImmediate = false);
	UFUNCTION()
	virtual void RemoveAllADSRequests(bool bImmediate = false);

	UFUNCTION(BlueprintCallable, Category = ADSCameraModifier, meta = (AdvancedDisplay = 3))
	static void CreateZoomRequest(ACorePlayerController* PlayerController, UObject* Requester, float ZoomAmount = 2.f, float ZoomRate = 4.f, float ZoomRateWithoutRequester = -1.f);
	UFUNCTION(BlueprintCallable, Category = ADSCameraModifier)
	static void ClearADSRequest(ACorePlayerController* PlayerController, UObject* Requester, bool bImmediate = false);
	UFUNCTION(BlueprintCallable, Category = ADSCameraModifier)
	static void ClearAllRequests(ACorePlayerController* PlayerController, bool bImmediate = false);

protected:
	UPROPERTY()
	TMap<TWeakObjectPtr<UObject>, FADSRequest> ZoomMap;
};
