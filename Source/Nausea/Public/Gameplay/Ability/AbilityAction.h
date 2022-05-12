// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Gameplay/AbilityObjectInterface.h"
#include "AbilityAction.generated.h"

UENUM(BlueprintType)
enum class EActionStage : uint8
{
	Invalid,
	Startup,
	Activation
};

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class NAUSEA_API UAbilityAction : public UObject
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UObject Interface
public:
	virtual UWorld* GetWorld() const override final;
//~ End UObject Interface

public:
	//Perform action immediately. Called only on CDO.
	virtual void PerformAction(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& AbilityInstance, const FAbilityTargetData& AbilityTargetData, EActionStage Stage) const {}

	//Initialize instance of action. Called only on instances.
	virtual void InitializeInstance(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& AbilityInstance, const FAbilityTargetData& AbilityTargetData, EActionStage Stage);
	void Tick(float DeltaTime) { if (!IsPendingKill() && bWantsTick) { TickAction(DeltaTime); } }
	virtual void Complete();
	virtual void Cleanup();

	FORCEINLINE EActionStage GetActionStage() const { return ActionStage; }
	FORCEINLINE bool IsCompleted() const { return bHasCompleted; }
	FORCEINLINE bool ShouldPerformOnAutonomous() const { return bPerformOnAutonomous; }

public:
	UFUNCTION(BlueprintCallable, Category = Action)
	UAbilityComponent* GetAbilityComponent() const { return OwningAbilityComponent.Get(); }

	UFUNCTION(BlueprintCallable, Category = Action)
	bool ShouldInstance() const { return bNeedsNewInstance; }

protected:
	virtual void TickAction(float DeltaTime) {}

protected:
	UPROPERTY(Transient)
	TWeakObjectPtr<UAbilityComponent> OwningAbilityComponent = nullptr;
	UPROPERTY(Transient)
	FAbilityInstanceHandle AbilityInstanceHandle;
	UPROPERTY(Transient)
	FAbilityTargetDataHandle AbilityTargetDataHandle;
	UPROPERTY(Transient)
	EActionStage ActionStage = EActionStage::Invalid;
	UPROPERTY(Transient)
	bool bHasCompleted = false;

	UPROPERTY()
	bool bNeedsNewInstance = false;
	UPROPERTY()
	bool bCompleteOnCleanup = false;
	UPROPERTY()
	bool bWantsTick = false;
	UPROPERTY()
	bool bPerformOnAutonomous = false;
};

class UCoreDamageType;

UCLASS()
class NAUSEA_API UAbilityActionDamage : public UAbilityAction
{
	GENERATED_UCLASS_BODY()

//~ Begin UAbilityAction Interface
public:
	virtual void PerformAction(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& AbilityInstance, const FAbilityTargetData& AbilityTargetData, EActionStage Stage) const override;
//~ End UAbilityAction Interface

protected:
	void ApplyDamageAtActor(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& AbilityInstance, const FAbilityTargetData& AbilityTargetData) const;
	void ApplyDamageAtLocation(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& AbilityInstance, const FAbilityTargetData& AbilityTargetData) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = Action)
	TSubclassOf<UCoreDamageType> DamageType = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Action)
	bool bIgnoreInstigator = false;
};

UCLASS()
class NAUSEA_API UAbilityActionDamageOverTime : public UAbilityActionDamage
{
	GENERATED_UCLASS_BODY()

//~ Begin UAbilityAction Interface
public:
	virtual void InitializeInstance(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& AbilityInstance, const FAbilityTargetData& AbilityTargetData, EActionStage Stage) override;
	virtual void Complete() override;
//~ End UAbilityAction Interface

protected:
	UFUNCTION()
	void ApplyDamageTimer();

protected:
	UPROPERTY(EditDefaultsOnly, Category = Action)
	float DamageTickDuration = 0.25f;

	UPROPERTY()
	FTimerHandle DamageTickHandle;
};