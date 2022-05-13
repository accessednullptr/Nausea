// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "NauseaGameInstance.generated.h"

class UWorld;
class UMapDataAsset;
class UCustomizationAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMapDataAssetListReadySignature, UNauseaGameInstance*, NauseaGameInstance, const TArray<UMapDataAsset*>&, MapDataAssetList);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCustomizationObjectListReadySignature, UNauseaGameInstance*, NauseaGameInstance, const TArray<UCustomizationAsset*>&, CustomizationObjectList);

/**
 * 
 */
UCLASS()
class NAUSEA_API UNauseaGameInstance : public UGameInstance
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UGameInstance Interface
protected:
	virtual void Init() override;
public:
	virtual void Shutdown() override;
//~ End UGameInstance Interface

public:
	UFUNCTION()
	bool LoadMapDataList(bool bImmediate);

	const TMap<TSoftObjectPtr<UWorld>, UMapDataAsset*>& GetMapDataAssetMap() const { return MapDataAssetMap; }
	UMapDataAsset* GetMapDataAsset(TSoftObjectPtr<UWorld> World) const;
	UMapDataAsset* GetMapDataAssetForCurrentWorld() const;

	UFUNCTION(BlueprintCallable, Category = GameInstance, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static const TMap<TSoftObjectPtr<UWorld>, UMapDataAsset*>& GetMapDataAssetMap(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = GameInstance, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static TArray<UMapDataAsset*> GetMapDataAssetList(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = GameInstance, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static UMapDataAsset* GetCurrentMapDataAsset(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = GameInstance, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static UMapDataAsset* GetMapDataAsset(const UObject* WorldContextObject, TSoftObjectPtr<UWorld> World);

	UFUNCTION()
	bool IsMapDataListReady() const { return bIsMapDataListReady; }

	UFUNCTION()
	bool LoadCustomizationList(bool bImmediate);
	UFUNCTION()
	bool IsCustomizationObjectListReady() const { return bIsCustomizationObjectListReady; }

	class UCustomizationMergeManager* GetCustomizationManager() const { return CustomizationManager; }

public:
	UPROPERTY()
	FMapDataAssetListReadySignature OnMapDataAssetListReady;
	UPROPERTY()
	FCustomizationObjectListReadySignature OnCustomizationObjectListReady;

protected:
	UFUNCTION()
	void CheckMapDataListLoadComplete();

	UFUNCTION()
	void CheckCustomizationObjectListLoadComplete();

protected:
	UPROPERTY()
	bool bIsMapDataListReady = false;
	UPROPERTY()
	TMap<TSoftObjectPtr<UWorld>, UMapDataAsset*> MapDataAssetMap;
	UPROPERTY()
	TArray<FPrimaryAssetId> PendingMapDataAssetList;
	
	UPROPERTY()
	bool bIsCustomizationObjectListReady = false;
	UPROPERTY()
	TArray<UCustomizationAsset*> CustomizationObjectList;
	UPROPERTY()
	TArray<FPrimaryAssetId> PendingCustomizationObjectAssetList;

	UPROPERTY(Transient, DuplicateTransient)
	UCustomizationMergeManager* CustomizationManager = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMapLoadAsyncActionSignature, bool, bResult);

UCLASS()
class NAUSEA_API UMapLoadAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UBlueprintAsyncActionBase Interface
	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;
//~ End UBlueprintAsyncActionBase Interface

public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", CallableWithoutWorldContext), Category = UI)
	static UMapLoadAsyncAction* RequestMapList(const UObject* WorldContextObject);

public:
	UPROPERTY(BlueprintAssignable)
	FMapLoadAsyncActionSignature OnMapDataListLoaded;

protected:
	UFUNCTION()
	void OnMapLoadRequestComplete(UNauseaGameInstance* NauseaGameInstance, const TArray<UMapDataAsset*>& MapDataAssetList);

protected:
	UPROPERTY()
	TWeakObjectPtr<UNauseaGameInstance> OwningGameInstance;
};