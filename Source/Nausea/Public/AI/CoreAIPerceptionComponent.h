// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Perception/AIPerceptionComponent.h"
#include "CoreAIPerceptionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGainedSightOfActorSignature, UCoreAIPerceptionComponent*, PerceptionComponent, AActor*, Actor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLostSightOfActorSignature, UCoreAIPerceptionComponent*, PerceptionComponent, AActor*, Actor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHeardNoiseFromActorSignature, UCoreAIPerceptionComponent*, PerceptionComponent, AActor*, Actor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FReceivedDamageFromActorSignature, UCoreAIPerceptionComponent*, PerceptionComponent, AActor*, Actor, float, DamageThreat);

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class NAUSEA_API UCoreAIPerceptionComponent : public UAIPerceptionComponent
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UActorComponent Interface
public:
	virtual void BeginPlay() override;
//~ End UActorComponent Interface

public:
	float GetMostRecentActiveStimulusAge(const FActorPerceptionInfo* PerceptionInfo) const;
	UFUNCTION(BlueprintCallable, Category = Perception)
	bool HasPerceivedActor(AActor* Actor, float MaxAge = 0.f) const;
	UFUNCTION(BlueprintCallable, Category = Perception)
	bool HasSeenActor(AActor* Actor, float MaxAge = 0.f) const;
	UFUNCTION(BlueprintCallable, Category = Perception)
	bool HasRecentlyHeardActor(AActor* Actor, float MaxAge = 5.f) const;
	UFUNCTION(BlueprintCallable, Category = Perception)
	bool HasRecentlyReceivedDamageFromActor(AActor* Actor, float MaxAge = 7.f) const;

	float GetStimulusAgeForActor(AActor* Actor, FAISenseID AISenseID) const;

	UFUNCTION(BlueprintCallable, Category = Perception)
	static bool IsUsableStimulus(const FAIStimulus& Stimulus) { return Stimulus.IsValid() && Stimulus.IsActive() && !Stimulus.IsExpired(); }

	const FActorPerceptionInfo* GetActorPerceptionInfo(const AActor* Actor) const { return GetPerceptualData().Find(TObjectKey<AActor>(Actor)); }

public:
	UPROPERTY(BlueprintAssignable)
	FGainedSightOfActorSignature OnGainedSightOfActor;
	UPROPERTY(BlueprintAssignable)
	FLostSightOfActorSignature OnLostSightOfActor;
	UPROPERTY(BlueprintAssignable)
	FHeardNoiseFromActorSignature OnHeardNoiseFromActor;
	UPROPERTY(BlueprintAssignable)
	FReceivedDamageFromActorSignature OnReceivedDamageFromActor;

	static FAISenseID AISenseSightID;
	static FAISenseID AISenseHearingID;
	static FAISenseID AISenseDamageID;

protected:
	UFUNCTION()
	virtual void OnPerceptionUpdate(AActor* Actor, FAIStimulus Stimulus);

	static void MakeNoise(AActor* NoiseMaker, float Loudness, APawn* NoiseInstigator, const FVector& NoiseLocation, float MaxRange, FName Tag);
};
