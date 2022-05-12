// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Player/CorePlayerState.h"
#include "NauseaPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerNameChangedSignature, ANauseaPlayerState*, PlayerState, const FString&, PlayerName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReadyChangedSignature, ANauseaPlayerState*, PlayerState, bool, bReady);

/**
 * 
 */
UCLASS()
class NAUSEA_API ANauseaPlayerState : public ACorePlayerState
{
	GENERATED_UCLASS_BODY()

//~ Begin APlayerState Interface
public:
	virtual void OnRep_PlayerName() override;
//~ End APlayerState Interface

public:
	UFUNCTION(BlueprintCallable, Category = PlayerState)
	bool IsReady() const { return bIsReady; }

	void SetIsReady(bool bInIsReady);

protected:
	UFUNCTION()
	virtual void OnRep_IsReady();

public:
	UPROPERTY(BlueprintAssignable, Category = PlayerState)
	FPlayerNameChangedSignature OnPlayerNameChanged;

	UPROPERTY(BlueprintAssignable, Category = PlayerState)
	FReadyChangedSignature OnReadyChanged;

private:
	//Has this player readied up.
	UPROPERTY(ReplicatedUsing = OnRep_IsReady)
	bool bIsReady = false;
};
