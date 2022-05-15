// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "System/NauseaGameInstance.h"
#include "System/NauseaAssetManager.h"
#include "System/NauseaWorldSettings.h"
#include "System/MapDataAsset.h"
#include "System/MeshMergeTypes.h"
#include "Character/Customization/CustomizationObject.h"

UNauseaGameInstance::UNauseaGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if !UE_SERVER
	CustomizationManager = CreateDefaultSubobject<UCustomizationMergeManager>(TEXT("CustomizationMergeManager"));
#endif //!UE_SERVER
}

void UNauseaGameInstance::Init()
{
	Super::OnStart();

	bIsMapDataListReady = true;
	LoadMapDataList(true);
	bIsCustomizationObjectListReady = true;
	LoadCustomizationList(true);
}

void UNauseaGameInstance::Shutdown()
{
	MapDataAssetMap.Empty();
	PendingMapDataAssetList.Empty();

	Super::Shutdown();
}

bool UNauseaGameInstance::LoadMapDataList(bool bImmediate)
{
	if (!bIsMapDataListReady)
	{
		return false;
	}

	bIsMapDataListReady = false;
	MapDataAssetMap.Empty(MapDataAssetMap.Num());

	UNauseaAssetManager* AssetManager = Cast<UNauseaAssetManager>(UAssetManager::GetIfValid());

	if (!AssetManager)
	{
		UE_LOG(LogTemp, Error, TEXT("UNauseaGameInstance::LoadMapDataList failed to load map data due to missing asset manager."));
		return false;
	}

	TArray<FPrimaryAssetId> MapDataAssetIdList;
	AssetManager->GetPrimaryAssetIdList(UMapDataAsset::MapDataAssetType, MapDataAssetIdList);

	TArray<FName> Bundles;
	Bundles.Add(FName("Game"));

	auto MapDataLoadComplete = [this](FPrimaryAssetId AssetId) {
		if (!PendingMapDataAssetList.Contains(AssetId))
		{
			return;
		}

		PendingMapDataAssetList.Remove(AssetId);

		UAssetManager* AssetManager = UAssetManager::GetIfValid();

		if (!AssetManager)
		{
			UE_LOG(LogTemp, Error, TEXT("UNauseaGameInstance::LoadMapDataList failed to load map data from primary asset Id %s due to a missing Asset Manager"), *AssetId.ToString());
			CheckMapDataListLoadComplete();
			return;
		}

		UMapDataAsset* LoadedMapData = AssetManager->GetPrimaryAssetObject<UMapDataAsset>(AssetId);

		if (!LoadedMapData)
		{
			UE_LOG(LogTemp, Error, TEXT("UNauseaGameInstance::LoadMapDataList failed to load map data from primary asset Id %s"), *AssetId.ToString());
			CheckMapDataListLoadComplete();
			return;
		}

		MapDataAssetMap.FindOrAdd(LoadedMapData->GetMapAsset()) = LoadedMapData;
		CheckMapDataListLoadComplete();
	};

	int32 StreamableDelegateDelay = 0;
	IConsoleVariable* StreamableDelegateDelayCVar = nullptr;
	if (bImmediate)
	{
		StreamableDelegateDelayCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("s.StreamableDelegateDelayFrames"));
		StreamableDelegateDelay = StreamableDelegateDelayCVar->GetInt();
		StreamableDelegateDelayCVar->Set(0);
		AssetManager->SetForceSynchronousLoadEnabled(true);
	}

	for (const FPrimaryAssetId& MapDataAssetId : MapDataAssetIdList)
	{
		PendingMapDataAssetList.Add(MapDataAssetId);
		AssetManager->LoadPrimaryAsset(MapDataAssetId, Bundles, FStreamableDelegate::CreateWeakLambda(this, MapDataLoadComplete, MapDataAssetId), FStreamableManager::AsyncLoadHighPriority);
	}

	if (bImmediate)
	{
		StreamableDelegateDelayCVar->Set(StreamableDelegateDelay);
		AssetManager->SetForceSynchronousLoadEnabled(false);
	}

	return true;
}

UMapDataAsset* UNauseaGameInstance::GetMapDataAsset(TSoftObjectPtr<UWorld> World) const
{
	if (MapDataAssetMap.Contains(World))
	{
		return MapDataAssetMap[World];
	}

	return nullptr;
}

inline UNauseaGameInstance* GetNauseaGameInstance(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World)
	{
		return nullptr;
	}

	return World->GetGameInstance<UNauseaGameInstance>();
}

UMapDataAsset* UNauseaGameInstance::GetMapDataAssetForCurrentWorld() const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	if (ANauseaWorldSettings* NauseaWorldSetting = Cast<ANauseaWorldSettings>(GetWorld()->GetWorldSettings()))
	{
		if (const TSoftObjectPtr<UMapDataAsset>& MapDataAssetObjectPtr = NauseaWorldSetting->GetMapDataAsset())
		{
			if (MapDataAssetObjectPtr.Get() != nullptr)
			{
				return MapDataAssetObjectPtr.Get();
			}
		}
	}

	FString CurrentMapPath = TSoftObjectPtr<UWorld>(GetWorld()).ToString();
	CurrentMapPath = CurrentMapPath.Replace(*GetWorld()->StreamingLevelsPrefix, *FString(""));
	return GetMapDataAsset(TSoftObjectPtr<UWorld>(CurrentMapPath));
}

const TMap<TSoftObjectPtr<UWorld>, UMapDataAsset*>& UNauseaGameInstance::GetMapDataAssetMap(const UObject* WorldContextObject)
{
	static const TMap<TSoftObjectPtr<UWorld>, UMapDataAsset*> InvalidMapDataAssetMap = TMap<TSoftObjectPtr<UWorld>, UMapDataAsset*>();

	UNauseaGameInstance* NauseaGameInstance = GetNauseaGameInstance(WorldContextObject);

	if (!NauseaGameInstance)
	{
		return InvalidMapDataAssetMap;
	}

	return NauseaGameInstance->MapDataAssetMap;
}

TArray<UMapDataAsset*> UNauseaGameInstance::GetMapDataAssetList(const UObject* WorldContextObject)
{
	UNauseaGameInstance* NauseaGameInstance = GetNauseaGameInstance(WorldContextObject);
	
	if (!NauseaGameInstance)
	{
		return TArray<UMapDataAsset*>();
	}

	TArray<UMapDataAsset*> MapDataAssetList;
	NauseaGameInstance->MapDataAssetMap.GenerateValueArray(MapDataAssetList);
	return MapDataAssetList;
}

UMapDataAsset* UNauseaGameInstance::GetCurrentMapDataAsset(const UObject* WorldContextObject)
{
	UNauseaGameInstance* NauseaGameInstance = GetNauseaGameInstance(WorldContextObject);

	if (!NauseaGameInstance)
	{
		return nullptr;
	}

	return NauseaGameInstance->GetMapDataAssetForCurrentWorld();
}

UMapDataAsset* UNauseaGameInstance::GetMapDataAsset(const UObject* WorldContextObject, TSoftObjectPtr<UWorld> World)
{
	UNauseaGameInstance* NauseaGameInstance = GetNauseaGameInstance(WorldContextObject);

	if (!NauseaGameInstance)
	{
		return nullptr;
	}

	return NauseaGameInstance->GetMapDataAsset(World);
}

bool UNauseaGameInstance::LoadCustomizationList(bool bImmediate)
{
	if (!bIsCustomizationObjectListReady)
	{
		return false;
	}

	bIsCustomizationObjectListReady = false;
	CustomizationObjectList.Reset();

	UNauseaAssetManager* AssetManager = Cast<UNauseaAssetManager>(UAssetManager::GetIfValid());

	if (!AssetManager)
	{
		UE_LOG(LogTemp, Error, TEXT("UNauseaGameInstance::LoadCustomizationList failed to load customization data due to missing asset manager."));
		return false;
	}

	TArray<FPrimaryAssetId> CustomizationObjectAssetList;

	AssetManager->GetPrimaryAssetIdList(UCustomizationAsset::CustomizationAssetType, CustomizationObjectAssetList);

	TArray<FName> Bundles;
	Bundles.Add(FName("Game"));

	auto CustomizationObjectLoadComplete = [this](FPrimaryAssetId AssetId) {
		if (!PendingCustomizationObjectAssetList.Contains(AssetId))
		{
			return;
		}

		PendingCustomizationObjectAssetList.Remove(AssetId);

		UAssetManager* AssetManager = UAssetManager::GetIfValid();

		if (!AssetManager)
		{
			UE_LOG(LogTemp, Error, TEXT("UNauseaGameInstance::LoadCustomizationList failed to load customization data from primary asset Id %s due to a missing Asset Manager"), *AssetId.ToString());
			CheckCustomizationObjectListLoadComplete();
			return;
		}

		UCustomizationAsset* LoadedCustomizationObject = AssetManager->GetPrimaryAssetObject<UCustomizationAsset>(AssetId);

		if (!LoadedCustomizationObject)
		{
			UE_LOG(LogTemp, Error, TEXT("UNauseaGameInstance::LoadCustomizationList failed to load customization data from primary asset Id %s"), *AssetId.ToString());
			CheckCustomizationObjectListLoadComplete();
			return;
		}

		CustomizationObjectList.Add(LoadedCustomizationObject);
		CheckCustomizationObjectListLoadComplete();
	};

	int32 StreamableDelegateDelay = 0;
	IConsoleVariable* StreamableDelegateDelayCVar = nullptr;
	if (bImmediate)
	{
		StreamableDelegateDelayCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("s.StreamableDelegateDelayFrames"));
		StreamableDelegateDelay = StreamableDelegateDelayCVar->GetInt();
		StreamableDelegateDelayCVar->Set(0);
		AssetManager->SetForceSynchronousLoadEnabled(true);
	}

	for (const FPrimaryAssetId& CustomizationAssetId : CustomizationObjectAssetList)
	{
		PendingCustomizationObjectAssetList.Add(CustomizationAssetId);
		AssetManager->LoadPrimaryAsset(CustomizationAssetId, Bundles, FStreamableDelegate::CreateWeakLambda(this, CustomizationObjectLoadComplete, CustomizationAssetId), FStreamableManager::AsyncLoadHighPriority);
	}

	if (bImmediate)
	{
		StreamableDelegateDelayCVar->Set(StreamableDelegateDelay);
		AssetManager->SetForceSynchronousLoadEnabled(false);
	}

	return true;
}

void UNauseaGameInstance::CheckMapDataListLoadComplete()
{
	if (PendingMapDataAssetList.Num() != 0)
	{
		return;
	}

	bIsMapDataListReady = true;

	TArray<UMapDataAsset*> MapDataAssetList;
	MapDataAssetMap.GenerateValueArray(MapDataAssetList);
	OnMapDataAssetListReady.Broadcast(this, MapDataAssetList);
}

void UNauseaGameInstance::CheckCustomizationObjectListLoadComplete()
{
	if (PendingMapDataAssetList.Num() != 0)
	{
		return;
	}

	bIsCustomizationObjectListReady = true;
	OnCustomizationObjectListReady.Broadcast(this, CustomizationObjectList);
}

UMapLoadAsyncAction::UMapLoadAsyncAction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UMapLoadAsyncAction::Activate()
{
	if (!OwningGameInstance.IsValid())
	{
		OnMapDataListLoaded.Broadcast(false);
		SetReadyToDestroy();
		return;
	}

	if (OwningGameInstance->IsMapDataListReady())
	{
		SetReadyToDestroy();
		OnMapDataListLoaded.Broadcast(true);
		return;
	}

	OwningGameInstance->OnMapDataAssetListReady.AddDynamic(this, &UMapLoadAsyncAction::OnMapLoadRequestComplete);
	
	OwningGameInstance->LoadMapDataList(false);
}

void UMapLoadAsyncAction::SetReadyToDestroy()
{
	OwningGameInstance->OnMapDataAssetListReady.RemoveDynamic(this, &UMapLoadAsyncAction::OnMapLoadRequestComplete);
	Super::SetReadyToDestroy();
}

UMapLoadAsyncAction* UMapLoadAsyncAction::RequestMapList(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World)
	{
		return nullptr;
	}

	UNauseaGameInstance* NauseaGameInstance = World->GetGameInstance<UNauseaGameInstance>();

	if (!NauseaGameInstance)
	{
		return nullptr;
	}

	UMapLoadAsyncAction* AsyncAction = NewObject<UMapLoadAsyncAction>();

	if (!AsyncAction)
	{
		return nullptr;
	}

	AsyncAction->OwningGameInstance = NauseaGameInstance;
	AsyncAction->RegisterWithGameInstance(NauseaGameInstance);
	return AsyncAction;
}

void UMapLoadAsyncAction::OnMapLoadRequestComplete(UNauseaGameInstance* NauseaGameInstance, const TArray<UMapDataAsset*>& MapDataAssetList)
{
	OnMapDataListLoaded.Broadcast(true);
	SetReadyToDestroy();
}