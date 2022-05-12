// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/CoreUserWidget.h"
#include "MainMenuScreenUserWidget.generated.h"

class AMainMenuGameState;
class UMapDataAsset;

/**
 * 
 */
UCLASS()
class NAUSEA_API UMainMenuScreenUserWidget : public UCoreUserWidget
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UUserWidget Interface
public:
	virtual bool Initialize() override;
//~ End UUserWidget Interface

protected:
	UFUNCTION()
	void OnPlayerStateAdded(bool bIsPlayer, ACorePlayerState* PlayerState);
	UFUNCTION()
	void OnPlayerStateRemoved(bool bIsPlayer, ACorePlayerState* PlayerState);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Widget", meta=(DisplayName = "On Player State Added", ScriptName = "OnPlayerStateAdded"))
	void K2_OnPlayerStateAdded(bool bIsPlayer, ACorePlayerState* PlayerState);
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Widget", meta=(DisplayName = "On Player State Removed", ScriptName = "OnPlayerStateRemoved"))
	void K2_OnPlayerStateRemoved(bool bIsPlayer, ACorePlayerState* PlayerState);

	UFUNCTION()
	void OnSelectedMapDataChanged(AMainMenuGameState* MainMenuGameState, const UMapDataAsset* SelectedMapData);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Widget", meta=(DisplayName = "On Selected Map Data Changed", ScriptName = "OnSelectedMapDataChanged"))
	void K2_OnSelectedMapDataChanged(const UMapDataAsset* SelectedMapData);
};
