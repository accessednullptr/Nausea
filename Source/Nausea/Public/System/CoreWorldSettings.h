// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "CoreWorldSettings.generated.h"

class UCoreGameModeSettings;

/**
 * 
 */
UCLASS()
class NAUSEA_API ACoreWorldSettings : public AWorldSettings
{
	GENERATED_UCLASS_BODY()
	
//~ Begin AWorldSettings Interface
public:
	virtual void NotifyBeginPlay() override;
//~ End AWorldSettings Interface

public:
	template<class T>
	T* GetGameModeSettings() const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UCoreGameModeSettings>::Value, "'T' template parameter to GetGameModeSettings must be derived from UCoreGameModeSettings");
		return Cast<T>(GameModeSettings);
	}

protected:
	UPROPERTY(EditDefaultsOnly, Instanced, Category = GameMode)
	UCoreGameModeSettings* GameModeSettings = nullptr;
};
