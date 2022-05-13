// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectKey.h"
#include "GameFramework/Actor.h"
#include "AI/EnemySelection/PerceptionEnemyComponent.h"
#include "AggroEnemyComponent.generated.h"

class IPlayerOwnershipInterface;

USTRUCT(BlueprintType)
struct FAggroData
{
	GENERATED_USTRUCT_BODY()

public:
	FAggroData() {}
	FAggroData(AActor* InActor, float InThreat);

	bool IsValid() const { return Actor.IsValid(); }
	float GetThreat() const { return Threat; }
	void AddThreat(float InThreat) { Threat -= InThreat; }

	bool TickAggroEntry(float DeltaTime, const bool bIsPerceived);

	bool operator<(const FAggroData& Other) const
{
		return Threat < Other.Threat;
	}

protected:
	UPROPERTY()
	TWeakObjectPtr<AActor> Actor = nullptr;
	UPROPERTY()
	float Threat = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAggroDataUpdateSignature, UAggroEnemyComponent*, EnemyComponent, const FAggroData&, AggroData);

/**
 * 
 */
UCLASS(meta = (BlueprintSpawnableComponent))
class NAUSEA_API UAggroEnemyComponent : public UPerceptionEnemyComponent
{
	GENERATED_UCLASS_BODY()

//~ Begin UActorComponent Interface
public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
//~ End UActorComponent Interface
	
//~ Begin UAIControllerComponent Interface
protected:
	virtual void OnPawnUpdated(ACoreAIController* AIController, ACoreCharacter* InCharacter) override;
//~ End UAIControllerComponent Interface
	
//~ Begin UEnemySelectionComponent Interface
public:
	virtual void GetAllEnemies(TArray<AActor*>& EnemyList) const override;
protected:
	virtual AActor* FindBestEnemy() const override;
//~ End UEnemySelectionComponent Interface

//~ Begin UPerceptionEnemyComponent Interface
protected:
	virtual void GainedVisibilityOfActor(UCoreAIPerceptionComponent* PerceptionComponent, AActor* Actor) override;
	virtual void LostVisiblityOfActor(UCoreAIPerceptionComponent* PerceptionComponent, AActor* Actor) override;
	virtual void HeardNoiseFromActor(UCoreAIPerceptionComponent* PerceptionComponent, AActor* Actor) override;
	virtual void DamageReceivedFromActor(UCoreAIPerceptionComponent* PerceptionComponent, AActor* Actor, float DamageThreat) override;
//~ End UPerceptionEnemyComponent Interface

public:
	UPROPERTY(BlueprintAssignable)
	FAggroDataUpdateSignature OnAddedAggroData;
	UPROPERTY(BlueprintAssignable)
	FAggroDataUpdateSignature OnRemovedAggroData;
	UPROPERTY(BlueprintAssignable)
	FAggroDataUpdateSignature OnHighestAggroThreatChanged;

protected:
	void CleanupAggroData();

	void UpdateActorAggro(AActor* Actor, float Threat = 1.f);

protected:
	TMap<TObjectKey<AActor>, FAggroData> AggroDataMap = TMap<TObjectKey<AActor>, FAggroData>();
	TArray<TObjectKey<AActor>> SortedAggroKeyList = TArray<TObjectKey<AActor>>();
};
