// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Character/CoreCustomizationComponent.h"
#include "Character/Customization/CustomizationObject.h"
#include "MainMenuCustomizationComponent.generated.h"

class USkeletalMeshComponent;

/**
 * 
 */
UCLASS()
class NAUSEA_API UMainMenuCustomizationComponent : public UCoreCustomizationComponent
{
	GENERATED_UCLASS_BODY()

//~ Begin UActorComponent Interface
protected:
	virtual void BeginPlay() override;
//~ End UActorComponent Interface

//~ Begin UCoreCustomizationComponent Interface
protected:
	virtual void UpdateCustomization() override;
//~ End UCoreCustomizationComponent Interface

protected:
	UPROPERTY()
	TMap<ECustomizationSlot, USkeletalMeshComponent*> SkeletalMeshBodyComponents = TMap<ECustomizationSlot, USkeletalMeshComponent*>();
};
