// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectKey.h"
#include "Camera/CameraModifier.h"
#include "InteractionCameraModifier.generated.h"

class ACorePlayerController;

/**
 * 
 */
UCLASS()
class NAUSEA_API UInteractionCameraModifier : public UCameraModifier
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UCameraModifier Interface
public:
	virtual void EnableModifier() override;
	virtual bool ProcessViewRotation(class AActor* ViewTarget, float DeltaTime, FRotator& OutViewRotation, FRotator& OutDeltaRot) override;
//~ End UCameraModifier Interface

	void RequestInteraction(UObject* Requester);
	void ClearInteraction(UObject* Requester);
	void ClearAllInteractions();

	UFUNCTION()
	static void CreateInteractionRequest(ACorePlayerController* PlayerController, UObject* Requester);
	UFUNCTION()
	static void ClearInteractionRequest(ACorePlayerController* PlayerController, UObject* Requester);
	UFUNCTION()
	static void ClearAllRequests(ACorePlayerController* PlayerController);

protected:
	void UpdateInteractionModifier();

protected:
	UPROPERTY(EditDefaultsOnly, Category = Interaction)
	FVector2D MagnetismStrength = FVector2D(0.5f, 10.f);

	UPROPERTY(EditDefaultsOnly, Category = Interaction)
	float YawLock = 15.f;
	UPROPERTY(EditDefaultsOnly, Category = Interaction)
	float PitchLock = 15.f;

	UPROPERTY(Transient)
	FRotator OriginalRotation = FRotator(0);

	TSet<TObjectKey<UObject>> RequestSet;
};
