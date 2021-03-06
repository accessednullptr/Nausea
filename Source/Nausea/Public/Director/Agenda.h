// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Agenda.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API UAgenda : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION()
	virtual void InitializeAgenda(bool bFromLoad = false);
};

UCLASS()
class NAUSEA_API UTask : public UObject
{
	GENERATED_UCLASS_BODY()

	virtual void InitializeTask(float PercentComplete);

	UPROPERTY()
	float ActionTimeStamp = 0.f;
};