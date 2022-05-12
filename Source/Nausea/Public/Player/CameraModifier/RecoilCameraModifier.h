// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "RecoilCameraModifier.generated.h"

class ACorePlayerController;

USTRUCT(BlueprintType)
struct NAUSEA_API FRecoilRequest
{
	GENERATED_USTRUCT_BODY()

	FRecoilRequest(){}

	FRecoilRequest(const FVector2D& InRecoil, float InRecoilRate)
	{
		Recoil = InRecoil;
		RecoilRate = InRecoilRate;
	}

public:
	void TickRecoil(float DeltaTime, FVector2D& AppliedRecoil)
	{
		if (!IsValid())
		{
			return;
		}

		RecoilProgress = FMath::Clamp(RecoilProgress + (DeltaTime * RecoilRate), 0.f, 1.f);

		const FVector2D NewRecoilDirection = FMath::InterpEaseOut(FVector2D(0), Recoil, RecoilProgress, 3.f);

		AppliedRecoil = NewRecoilDirection - LastAppliedRecoil;

		LastAppliedRecoil = NewRecoilDirection;
	}

	void UpdateProperties(const FRecoilRequest& InProperties)
	{
		Recoil = InProperties.Recoil;
		RecoilRate = InProperties.RecoilRate;
	}
	
	bool IsValid() const { return Recoil.X != -MAX_FLT && Recoil.Y != -MAX_FLT; }
	bool PendingRemoval() const { return RecoilProgress >= 1.f; }

protected:
	UPROPERTY()
	FVector2D Recoil = FVector2D(-MAX_FLT);
	UPROPERTY()
	FVector2D LastAppliedRecoil = FVector2D(0);
	UPROPERTY()
	float RecoilRate = 1.f;
	UPROPERTY()
	float RecoilProgress = 0.f;
};

/**
 * 
 */
UCLASS()
class NAUSEA_API URecoilCameraModifier : public UCameraModifier
{
	GENERATED_UCLASS_BODY()

public:
	virtual bool ProcessViewRotation(class AActor* ViewTarget, float DeltaTime, FRotator& OutViewRotation, FRotator& OutDeltaRot) override;

	UFUNCTION()
	virtual void AddRecoilRequest(const FRecoilRequest& RecoilRequest);
	UFUNCTION()
	virtual void RemoveAllRecoilRequests();

	UFUNCTION(BlueprintCallable, Category = ADSCameraModifier, meta = (AdvancedDisplay = 3))
	static void CreateRecoilRequest(ACorePlayerController* PlayerController, const FVector2D& Recoil, float RecoilDuration = 0.25f);
	UFUNCTION(BlueprintCallable, Category = ADSCameraModifier)
	static void ClearAllRecoilRequests(ACorePlayerController* PlayerController);

protected:
	UPROPERTY()
	TArray<FRecoilRequest> RecoilList;
};
