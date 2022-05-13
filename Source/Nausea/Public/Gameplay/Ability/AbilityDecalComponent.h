// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/DecalComponent.h"
#include "Gameplay/Ability/AbilityTypes.h"
#include "Gameplay/AbilityObjectInterface.h"
#include "AbilityDecalComponent.generated.h"

class UAbilityInfo;

UCLASS(Blueprintable, BlueprintType)
class NAUSEA_API UAbilityDecalComponent : public UDecalComponent, public IAbilityObjectInterface
{
	GENERATED_UCLASS_BODY()

//~ Begin IAbilityObjectInterface Interface
public:
	virtual void InitializeForAbilityData(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& AbilityInstance, const FAbilityTargetData& AbilityData) override;
	virtual void OnAbilityActivate() override;
	virtual void OnAbilityComplete() override;
	virtual void OnAbilityCleanup() override;
//~ End IAbilityObjectInterface Interface

public:
	static const FName StartupBeginTimeScalarParameter;
	static const FName StartupEndTimeScalarParameter;
	static const FName ActivationBeginTimeScalarParameter;
	static const FName ActivationEndTimeScalarParameter;

protected:
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* DynamicDecalMaterial = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category = Decal)
	bool bShowStartupTime = true;
	UPROPERTY(EditDefaultsOnly, Category = Decal)
	bool bShowActivationTime = true;

public:
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static UAbilityDecalComponent* SpawnAbilityDecal(const UObject* WorldContextObject, TSubclassOf<UAbilityDecalComponent> AbilityDecalClass, const FAbilityTargetData& AbilityTargetData);
};