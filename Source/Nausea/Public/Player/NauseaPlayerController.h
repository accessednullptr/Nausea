// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Player/CorePlayerController.h"
#include "NauseaPlayerController.generated.h"

/**
 * 
 */
UCLASS(Config=Game)
class NAUSEA_API ANauseaPlayerController : public ACorePlayerController
{
	GENERATED_UCLASS_BODY()

	//Any children of this sort of action has full access to this class' protected members.
	friend class APlayerControllerAsyncAction;

//~ Begin APlayerController Interface
public:
	virtual void SpawnPlayerCameraManager() override;
//~ End APlayerController Interface

public:
	UFUNCTION(BlueprintCallable, Category = PlayerController)
	ANauseaPlayerCameraManager* GetNauseaPlayerCameraManager() const;

	UFUNCTION(BlueprintCallable, Category = PlayerController)
	virtual void SetIsReady(bool bIsReady);

protected:
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Reliable_SetIsReady(bool bIsReady);

protected:
	UPROPERTY()
	FTimerHandle DeathCameraTimer;

private:
	UPROPERTY()
	ANauseaPlayerCameraManager* NauseaPlayerCameraManager = nullptr;
};