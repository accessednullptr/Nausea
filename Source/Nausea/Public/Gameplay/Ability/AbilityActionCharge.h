// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Ability/AbilityAction.h"
#include "AbilityActionCharge.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API UAbilityActionSlowRotation : public UAbilityAction
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UAbilityAction Interface
public:
	virtual void InitializeInstance(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& AbilityInstance, const FAbilityTargetData& AbilityTargetData, EActionStage Stage) override;
	virtual void Cleanup() override;
//~ End UAbilityAction Interface

protected:
	FDelegateHandle RotationRateDelegateHandle = FDelegateHandle();
	
	UPROPERTY(Transient)
	FTimerHandle SlowDownTimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = Action)
	bool bProgressivelySlowRotation = true;

	UPROPERTY(EditDefaultsOnly, Category = Action)
	float RotationMultiplier = 0.f;
};

UCLASS()
class NAUSEA_API UAbilityActionCharge : public UAbilityAction
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UAbilityAction Interface
public:
	virtual void InitializeInstance(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& AbilityInstance, const FAbilityTargetData& AbilityTargetData, EActionStage Stage) override;
	virtual void Complete() override;
protected:
	virtual void TickAction(float DeltaTime) override;
//~ End UAbilityAction Interface
};
