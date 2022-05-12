// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "System/CoreGameState.h"
#include "MainMenuGameState.generated.h"

class UMapDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSelectedMapDataChangedSignature, AMainMenuGameState*, GameState, const UMapDataAsset*, SelectedMapData);

/**
 * 
 */
UCLASS()
class NAUSEA_API AMainMenuGameState : public ACoreGameState
{
	GENERATED_UCLASS_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = MainMenuGameState)
	UMapDataAsset* GetSelectedMapData() const { return LoadedSelectedMapData; }

	UFUNCTION(BlueprintCallable, Category = MainMenuGameState)
	bool SetSelectedMapData(TSoftObjectPtr<UMapDataAsset> InSelectedMapData);

public:
	UPROPERTY(BlueprintAssignable, Category = MainMenuGameState)
	FSelectedMapDataChangedSignature OnSelectedMapDataChanged;

protected:
	UFUNCTION()
	void OnRep_SelectedMapData();

protected:
	UPROPERTY(ReplicatedUsing = OnRep_SelectedMapData)
	TSoftObjectPtr<UMapDataAsset> SelectedMapData = nullptr;
	UPROPERTY(Transient)
	UMapDataAsset* LoadedSelectedMapData = nullptr;
};
