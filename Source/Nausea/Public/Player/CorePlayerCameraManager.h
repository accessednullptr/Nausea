// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "Player/PlayerOwnershipInterface.h"
#include "CorePlayerCameraManager.generated.h"

class UADSCameraModifier;
class URecoilCameraModifier;
class UInteractionCameraModifier;

//Redefinition of APlayerCameraManager's camera styles in a way that is accessible
namespace CameraStyleName
{
	extern NAUSEA_API const FName NAME_Default;
	extern NAUSEA_API const FName NAME_Fixed;
	extern NAUSEA_API const FName NAME_ThirdPerson;
	extern NAUSEA_API const FName NAME_FreeCam;
	extern NAUSEA_API const FName NAME_FreeCam_Default;
	extern NAUSEA_API const FName NAME_FirstPerson;
}

UENUM(BlueprintType)
enum class ECameraMode : uint8
{
	FirstPerson,
	ThirdPerson,
	FreeCamera
};

/**
 * 
 */
UCLASS()
class NAUSEA_API ACorePlayerCameraManager : public APlayerCameraManager, public IPlayerOwnershipInterface
{
	GENERATED_UCLASS_BODY()
	
//~ Begin IPlayerOwnershipInterface Interface
public:
	virtual ACorePlayerState* GetOwningPlayerState() const override;
//~ End IPlayerOwnershipInterface Interface

//~ Begin APlayerCameraManager Interface
public:
	virtual void InitializeFor(APlayerController* PC) override;
	virtual void SetViewTarget(AActor* NewViewTarget, FViewTargetTransitionParams TransitionParams = FViewTargetTransitionParams()) override;
	virtual void AssignViewTarget(AActor* NewTarget, FTViewTarget& VT, struct FViewTargetTransitionParams TransitionParams = FViewTargetTransitionParams()) override;
protected:
	virtual void UpdateViewTarget(FTViewTarget& OutViewTarget, float DeltaTime) override;
//~ End APlayerCameraManager Interface

public:
	UFUNCTION(BlueprintCallable, Category = PlayerCameraManager)
	ACorePlayerController* GetPlayerController() const;

	UFUNCTION(BlueprintCallable, Category = PlayerCameraManager)
	UADSCameraModifier* GetADSCameraModifier() const { return ADSCameraModifier; }
	UFUNCTION(BlueprintCallable, Category = PlayerCameraManager)
	URecoilCameraModifier* GetRecoilCameraModifier() const { return RecoilCameraModifier; }
	UFUNCTION(BlueprintCallable, Category = PlayerCameraManager)
	UInteractionCameraModifier* GetInteractionCameraModifier() const { return InteractionCameraModifier; }

	UFUNCTION(BlueprintCallable, Category = PlayerCameraManager)
	bool IsDefaultCamera() const { return CameraStyle == CameraStyleName::NAME_Default; }
	UFUNCTION(BlueprintCallable, Category = PlayerCameraManager)
	bool IsFixedCamera() const { return CameraStyle == CameraStyleName::NAME_Fixed; }
	UFUNCTION(BlueprintCallable, Category = PlayerCameraManager)
	bool IsThirdPersonCamera() const { return CameraStyle == CameraStyleName::NAME_ThirdPerson; }
	UFUNCTION(BlueprintCallable, Category = PlayerCameraManager)
	bool IsFreeCamera() const { return CameraStyle == CameraStyleName::NAME_FreeCam; }
	UFUNCTION(BlueprintCallable, Category = PlayerCameraManager)
	bool IsDefaultFreeCamera() const { return CameraStyle == CameraStyleName::NAME_FreeCam_Default; }
	UFUNCTION(BlueprintCallable, Category = PlayerCameraManager)
	bool IsFirstPersonCamera() const { return CameraStyle == CameraStyleName::NAME_FirstPerson; }

	UFUNCTION()
	void SetCameraStyle(const FName& InCameraStyle, bool bUpdateViewTarget = true);
	
protected:
	UFUNCTION()
	void PawnPossesed(ACorePlayerController* PlayerController, ACoreCharacter* Pawn);
	UFUNCTION()
	void PossessedPawnKilled(ACoreCharacter* KilledCharacter, TSubclassOf<UDamageType> DamageType, float Damage, APlayerState* KillerPlayerState);

	UFUNCTION(BlueprintCallable, Category = PlayerCameraManager)
	void SetCameraMode(ECameraMode CameraMode);

	UFUNCTION(BlueprintImplementableEvent, Category = PlayerCameraManager)
	void OnInitializeCameraManager(ACorePlayerController* PlayerController);
	UFUNCTION(BlueprintImplementableEvent, Category = PlayerCameraManager)
	void OnPawnPossessed(ACoreCharacter* Pawn);
	UFUNCTION(BlueprintImplementableEvent, Category = PlayerCameraManager)
	void OnPossessedPawnKilled(ACoreCharacter* KilledCharacter, TSubclassOf<UDamageType> DamageType, float Damage, APlayerState* KillerPlayerState);

	UFUNCTION()
	void PostUpdateViewTarget(FTViewTarget& OutViewTarget, bool bApplyModifiers, float DeltaTime);

protected:
	UPROPERTY()
	UADSCameraModifier* ADSCameraModifier = nullptr;
	UPROPERTY()
	URecoilCameraModifier* RecoilCameraModifier = nullptr;
	UPROPERTY()
	UInteractionCameraModifier* InteractionCameraModifier = nullptr;
};
