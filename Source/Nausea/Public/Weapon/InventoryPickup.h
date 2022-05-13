// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InventoryPickup.generated.h"

UCLASS()
class NAUSEA_API AInventoryPickup : public AActor
{
	GENERATED_UCLASS_BODY()

	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
