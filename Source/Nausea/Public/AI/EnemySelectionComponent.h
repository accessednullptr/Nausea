// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIControllerComponent.h"
#include "EnemySelectionComponent.generated.h"

class IAITargetInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FEnemyChangedSignature, UEnemySelectionComponent*, EnemySelectionComponent, AActor*, NewEnemy, AActor*, PreviousEnemy);

/*
* Base class for all enemy selection AI systems.
*/
UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class NAUSEA_API UEnemySelectionComponent : public UAIControllerComponent
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UAIControllerComponent Interface
protected:
	virtual void OnPawnUpdated(ACoreAIController* AIController, ACoreCharacter* InCharacter) override;
//~ End UAIControllerComponent Interface

public:
	UFUNCTION(BlueprintCallable, Category = EnemySelectionComponent)
	ACoreCharacter* GetOwningCharacter() const { return CurrentCharacter; }

	UFUNCTION(BlueprintCallable, Category = EnemySelectionComponent)
	AActor* GetEnemy() const;

	UFUNCTION()
	virtual AActor* FindBestEnemy() const;

	IAITargetInterface* GetEnemyInterface() const;

	//Attempts to set enemy to New Enemy. Returns false if fails.
	//If nullptr is passed as New Enemy, will attempt to set enemy as result of FindBestEnemy() instead.
	UFUNCTION(BlueprintCallable, Category = EnemySelectionComponent)
	virtual bool SetEnemy(AActor* NewEnemy, bool bForce = false);
	//Will clear out current enemy. Does not guarantee enemy will stay cleared, however.
	UFUNCTION(BlueprintCallable, Category = EnemySelectionComponent)
	virtual bool ClearEnemy(bool bForce = false);
	UFUNCTION(BlueprintCallable, Category = EnemySelectionComponent)
	virtual bool CanChangeEnemy() const;
	UFUNCTION(BlueprintCallable, Category = EnemySelectionComponent)
	virtual bool CanTargetEnemy(AActor* NewEnemy) const;
	UFUNCTION()
	virtual void EnemyChanged(AActor* PreviousEnemy);
	UFUNCTION(BlueprintImplementableEvent, Category = EnemySelectionComponent, meta = (DisplayName = "On Enemy Changed"))
	void K2_OnEnemyChanged(AActor* PreviousEnemy);

	UFUNCTION(BlueprintCallable, Category = EnemySelectionComponent)
	float GetEnemyChangeCooldownRemaining() const;
	UFUNCTION(BlueprintCallable, Category = EnemySelectionComponent)
	bool IsEnemyChangeCooldownActive() const;

	UFUNCTION(BlueprintCallable, Category = EnemySelectionComponent)
	bool LockEnemySelection(UObject* Requester = nullptr);
	UFUNCTION(BlueprintCallable, Category = EnemySelectionComponent)
	bool UnlockEnemySelection(UObject* Requester = nullptr);
	UFUNCTION(BlueprintCallable, Category = EnemySelectionComponent)
	void ClearEnemySelectionLock();
	UFUNCTION(BlueprintCallable, Category = EnemySelectionComponent)
	bool IsEnemySelectionLocked() const;

	//Returns a list of known enemies (not to be confused with currently targeted enemy). Should be in priority order.
	UFUNCTION(BlueprintCallable, Category = EnemySelectionComponent)
	virtual void GetAllEnemies(TArray<AActor*>& EnemyList) const;

public:
	UPROPERTY(BlueprintAssignable, Category = EnemySelectionComponent)
	FEnemyChangedSignature OnEnemyChanged;

protected:
	UFUNCTION()
	virtual void OnEnemyTargetableStateChanged(AActor* Actor, bool bTargetable);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = EnemySelectionComponent)
	bool bFindEnemyOnPosses = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = EnemySelectionComponent, meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bEnemyChangeCooldown = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = EnemySelectionComponent, meta = (EditCondition = bEnemyChangeCooldown))
	float EnemyChangeCooldown = 4.f;

private:
	UPROPERTY()
	ACoreCharacter* CurrentCharacter = nullptr;

	UPROPERTY(Transient)
	TScriptInterface<IAITargetInterface> CurrentEnemy = TScriptInterface<IAITargetInterface>(nullptr);

	UPROPERTY()
	FTimerHandle EnemyCooldownTimerHandle;

	UPROPERTY()
	TSet<UObject*> EnemySelectionLockSet;
};
