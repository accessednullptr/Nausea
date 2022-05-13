// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Gameplay/Ability/AbilityTypes.h"
#include "AbilityObjectInterface.generated.h"

class UAbilityComponent;
class UAbilityInfo;

UINTERFACE(MinimalAPI)
class UAbilityObjectInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class NAUSEA_API IAbilityObjectInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	virtual void InitializeForAbilityData(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& AbilityInstance, const FAbilityTargetData& AbilityTargetData) { K2_InitializeForAbilityData(AbilityComponent, AbilityInstance, AbilityTargetData); }

	virtual void OnAbilityActivate() { return Execute_K2_OnAbilityActivate(reinterpret_cast<UObject*>(this)); }

	virtual void OnAbilityComplete() { return Execute_K2_OnAbilityComplete(reinterpret_cast<UObject*>(this)); }

	//Called when either an ability has been interrupted or has been cleaned up. Essentially a notification that our target data has been removed from the owning ability component.
	virtual void OnAbilityCleanup() { return Execute_K2_OnAbilityCleanup(reinterpret_cast<UObject*>(this)); }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = AbilityObjectInterface, meta = (DisplayName="Initialize For Location Target Data"))
	void K2_InitializeForAbilityData(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& AbilityInstance, const FAbilityTargetData& AbilityTargetData);
	
	UFUNCTION(BlueprintImplementableEvent, Category = AbilityObjectInterface, meta = (DisplayName="On Ability Activate"))
	void K2_OnAbilityActivate();

	UFUNCTION(BlueprintImplementableEvent, Category = AbilityObjectInterface, meta = (DisplayName="On Ability Complete"))
	void K2_OnAbilityComplete();

	UFUNCTION(BlueprintImplementableEvent, Category = AbilityObjectInterface, meta = (DisplayName="On Ability Cleanup"))
	void K2_OnAbilityCleanup();
};

UCLASS(Abstract, MinimalAPI)
class UAbilityObjectInterfaceStatics : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()
};