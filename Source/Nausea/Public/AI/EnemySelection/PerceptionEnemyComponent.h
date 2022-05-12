// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Perception/AIPerceptionTypes.h"
#include "AI/EnemySelectionComponent.h"
#include "PerceptionEnemyComponent.generated.h"

class UCoreAIPerceptionComponent;

/**
 * 
 */
UCLASS(meta = (BlueprintSpawnableComponent))
class NAUSEA_API UPerceptionEnemyComponent : public UEnemySelectionComponent
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UActorComponent Interface
protected:
	virtual void BeginPlay() override;
//~ End UActorComponent Interface

//~ Begin UEnemySelectionComponent Interface
public:
	virtual void GetAllEnemies(TArray<AActor*>& EnemyList) const override;
protected:
	virtual AActor* FindBestEnemy() const override;
//~ End UEnemySelectionComponent Interface

public:
	UFUNCTION(BlueprintCallable, Category = EnemySelectionComponent)
	UCoreAIPerceptionComponent* GetPerceptionComponent() const { return OwningPerceptionComponent; }

protected:
	UFUNCTION()
	virtual void GainedVisibilityOfActor(UCoreAIPerceptionComponent* PerceptionComponent, AActor* Actor);
	UFUNCTION()
	virtual void LostVisiblityOfActor(UCoreAIPerceptionComponent* PerceptionComponent, AActor* Actor);
	UFUNCTION()
	virtual void HeardNoiseFromActor(UCoreAIPerceptionComponent* PerceptionComponent, AActor* Actor);
	UFUNCTION()
	virtual void DamageReceivedFromActor(UCoreAIPerceptionComponent* PerceptionComponent, AActor* Actor, float DamageThreat);

private:
	UPROPERTY(Transient)
	UCoreAIPerceptionComponent* OwningPerceptionComponent = nullptr;
};
