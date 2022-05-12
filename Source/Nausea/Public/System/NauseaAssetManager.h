// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "NauseaAssetManager.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API UNauseaAssetManager : public UAssetManager
{
	GENERATED_BODY()
	
//~ Begin UAssetManager Interface
public:
	virtual void StartInitialLoading() override;
//~ End UAssetManager Interface

public:
	void SetForceSynchronousLoadEnabled(bool bEnable) { bShouldUseSynchronousLoad = bEnable; }

};
