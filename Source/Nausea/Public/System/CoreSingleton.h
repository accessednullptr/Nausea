// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Tickable.h"
#include "GameplayTags/Classes/GameplayTagContainer.h"
#include "CoreSingleton.generated.h"

class UGameInstance;
class USkeletalMesh;
class UCustomizationMergeManager;

DECLARE_STATS_GROUP(TEXT("CoreCustomizationComponent"), STATGROUP_Customization, STATCAT_Advanced);

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSingletonTickSignature, float, DeltaTime);

/**
 * 
 */
UCLASS()
class NAUSEA_API UCoreSingleton : public UObject, public FTickableGameObject
{
	GENERATED_UCLASS_BODY()
	
//~ Begin FTickableGameObject Interface
protected:
	virtual void Tick(float DeltaTime) override;
public:
	virtual ETickableTickType GetTickableTickType() const { return ETickableTickType::Always; }
	virtual bool IsTickable() const { return !IsPendingKill(); }
	virtual TStatId GetStatId() const { return TStatId(); }
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
//~ End FTickableGameObject Interface

public:
	UFUNCTION(BlueprintCallable, Category = Singleton, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static void BindToSingletonTick(const UObject* WorldContextObject, FOnSingletonTickSignature Delegate);
	UFUNCTION(BlueprintCallable, Category = Singleton, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static void UnbindFromSingletonTick(const UObject* WorldContextObject, FOnSingletonTickSignature Delegate);

	static void RegisterActorWithTag(const UObject* WorldContextObject, AActor* Actor, const FGameplayTagContainer& Tag);
	static void UnregisterActorWithTag(const UObject* WorldContextObject, AActor* Actor, const FGameplayTagContainer& Tag);
	static void GetActorsWithTag(const UObject* WorldContextObject, TArray<AActor*>& ActorList, const FGameplayTagContainer& Tag);

	UFUNCTION(BlueprintCallable, Category = Singleton, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext, DisplayName = "Register Actor With Tag"))
	static void K2_RegisterActorWithTag(const UObject* WorldContextObject, AActor* Actor, FGameplayTagContainer Tag) { RegisterActorWithTag(WorldContextObject, Actor, Tag); }
	UFUNCTION(BlueprintCallable, Category = Singleton, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext, DisplayName = "Unregister Actor With Tag"))
	static void K2_UnregisterActorWithTag(const UObject* WorldContextObject, AActor* Actor, FGameplayTagContainer Tag) { UnregisterActorWithTag(WorldContextObject, Actor, Tag); }
	UFUNCTION(BlueprintCallable, Category = Singleton, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext, DisplayName = "Get Actors With Tag"))
	static TArray<AActor*> K2_GetActorsWithTag(const UObject* WorldContextObject, FGameplayTagContainer Tag)
	{
		static TArray<AActor*> ResultList;
		GetActorsWithTag(WorldContextObject, ResultList, Tag);
		return ResultList;
	}

private:
	UPROPERTY(Transient, DuplicateTransient)
	TArray<FOnSingletonTickSignature> SingletonTickCallbackList;

	UPROPERTY(Transient)
	TWeakObjectPtr<UGameInstance> GameInstance = nullptr;

	TMap<FGameplayTag, TArray<TWeakObjectPtr<AActor>>> ActorGameplayTagMap = TMap<FGameplayTag, TArray<TWeakObjectPtr<AActor>>>();
};