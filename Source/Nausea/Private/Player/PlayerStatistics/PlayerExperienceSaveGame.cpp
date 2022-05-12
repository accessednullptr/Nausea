// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerStatistics/PlayerExperienceSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/LocalPlayer.h"
#include "Player/CorePlayerController.h"
#include "Player/CorePlayerState.h"

UPlayerExperienceSaveGame::UPlayerExperienceSaveGame(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UPlayerExperienceSaveGame::MarkFromLoad()
{
	SaveGameType = ESaveGameType::FromLoad;
}

void UPlayerExperienceSaveGame::MarkFromRPC()
{
	SaveGameType = ESaveGameType::FromRPC;
}

uint64 UPlayerExperienceSaveGame::GetExperienceValue(TSoftClassPtr<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant) const
{
	return Experience.GetValue(PlayerClass, Variant);
}

uint64 UPlayerExperienceSaveGame::SetExperienceValue(TSoftClassPtr<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, uint64 InValue)
{
	return Experience.SetValue(PlayerClass, Variant, InValue);
}

uint64 UPlayerExperienceSaveGame::AddExperienceValue(TSoftClassPtr<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, uint64 Delta)
{
	return Experience.AddValue(PlayerClass, Variant, Delta);
}

bool UPlayerExperienceSaveGame::UpdatePlayerClassSelection(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant)
{
	bool bChangeDetected = GetPlayerSelectionDataMutable().SetSelectedPlayerClass(TSoftClassPtr<UPlayerClassComponent>(PlayerClass));
	bool bVariantChangeDetected = UpdatePlayerClassVariantSelection(PlayerClass, Variant);

	if (bChangeDetected)
	{
		OnSelectedPlayerClassChanged.Broadcast(PlayerClass);
	}

	return bVariantChangeDetected || bChangeDetected;
}

bool UPlayerExperienceSaveGame::UpdatePlayerClassVariantSelection(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant)
{
	bool bChangeDetected = false;

	TSoftClassPtr<UPlayerClassComponent> SoftPlayerClass = TSoftClassPtr<UPlayerClassComponent>(PlayerClass);
	if (!GetPlayerSelectionData().GetPlayerClassVariantSelection().Contains(SoftPlayerClass))
	{
		bChangeDetected |= true;
	}
	else if (GetPlayerSelectionData().GetPlayerClassVariantSelection()[SoftPlayerClass] != Variant)
	{
		bChangeDetected |= true;
	}

	GetPlayerSelectionDataMutable().GetPlayerClassVariantSelectionMutable().FindOrAdd(SoftPlayerClass) = Variant;

	if (bChangeDetected)
	{
		OnSelectedPlayerClassVariantChanged.Broadcast(PlayerClass, Variant);
	}

	return bChangeDetected;
}

bool UPlayerExperienceSaveGame::UpdatePlayerInventorySelectionData(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, const TArray<TSubclassOf<UInventory>>& InInventorySelection)
{
	bool bChangeDetected = false;

	TSoftClassPtr<UPlayerClassComponent> SoftPlayerClass = TSoftClassPtr<UPlayerClassComponent>(PlayerClass);

	if (!GetPlayerSelectionData().GetInventorySelection().Contains(SoftPlayerClass))
	{
		bChangeDetected |= true;
	}

	FPlayerClassSelectionMap& PlayerClassInventorySelection = GetPlayerSelectionDataMutable().GetInventorySelectionMutable().FindOrAdd(SoftPlayerClass);

	//Convert to soft class pointer for storage.
	TArray<TSoftClassPtr<UInventory>> SoftClassPointerWeaponList;
	SoftClassPointerWeaponList.Reserve(InInventorySelection.Num());
	for (const TSubclassOf<UInventory>& InventoryClass : InInventorySelection)
	{
		SoftClassPointerWeaponList.Add(TSoftClassPtr<UInventory>(InventoryClass));
	}

	if (!PlayerClassInventorySelection.Contains(Variant) || PlayerClassInventorySelection[Variant].GetSoftClassList() != SoftClassPointerWeaponList)
	{
		bChangeDetected |= true;
	}
	else
	{
		const TSet<TSoftClassPtr<UInventory>>& CurrentSelection = TSet<TSoftClassPtr<UInventory>>(SoftClassPointerWeaponList);
		const TSet<TSoftClassPtr<UInventory>>& PlayerClassSelection = TSet<TSoftClassPtr<UInventory>>(PlayerClassInventorySelection[Variant].GetSoftClassList());

		bChangeDetected |= CurrentSelection.Difference(PlayerClassSelection).Num() > 0;
	}

	PlayerClassInventorySelection.FindOrAdd(Variant) = SoftClassPointerWeaponList;

	return bChangeDetected;
}

const TArray<TSoftClassPtr<UInventory>>& UPlayerExperienceSaveGame::GetPlayerInventorySelection(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant) const
{
	TSoftClassPtr<UPlayerClassComponent> SoftPlayerClass = TSoftClassPtr<UPlayerClassComponent>(PlayerClass);
	if (!GetPlayerSelectionData().GetInventorySelection().Contains(SoftPlayerClass))
	{
		return FInventorySelectionEntry::InvalidConfigArray;
	}

	const FPlayerClassSelectionMap& PlayerClassSelection = GetPlayerSelectionData().GetInventorySelection()[SoftPlayerClass];
	
	if (PlayerClassSelection.Contains(Variant))
	{
		return PlayerClassSelection[Variant].GetSoftClassList();
	}

	return FInventorySelectionEntry::InvalidConfigArray;
}

FString UPlayerExperienceSaveGame::GetPlayerID(ACorePlayerController* PlayerController)
{
	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();

	if (!LocalPlayer)
	{
		return "";
	}

#if WITH_EDITOR
	FString SlotName;
	if (GEditor)
	{
		const int32 ID = LocalPlayer->GetWorld() ? (LocalPlayer->GetWorld()->GetOutermost() ? LocalPlayer->GetWorld()->GetOutermost()->PIEInstanceID : 0) : 0;
		SlotName = FString::Printf(TEXT("EditorClient%i"), ID);
	}
	else
	{
		SlotName = LocalPlayer->GetPreferredUniqueNetId().ToString();
	}
#else
	const FString SlotName = LocalPlayer->GetPreferredUniqueNetId().ToString();
#endif //WITH_EDITOR

	return SlotName;
}

UPlayerExperienceSaveGame* UPlayerExperienceSaveGame::LoadPlayerSaveData(ACorePlayerController* PlayerController)
{
	if (!PlayerController || !PlayerController->IsLocalPlayerController())
	{
		return nullptr;
	}

	const FString SlotName = GetPlayerID(PlayerController);

	if (SlotName == "")
	{
		UPlayerExperienceSaveGame* SaveGame = Cast<UPlayerExperienceSaveGame>(UGameplayStatics::CreateSaveGameObject(UPlayerExperienceSaveGame::StaticClass()));
		SaveGame->MarkFromLoad();
		SaveGame->LoadedSlotName = "";
		return SaveGame;
	}

	UPlayerExperienceSaveGame* SaveGame = nullptr;

	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		SaveGame = Cast<UPlayerExperienceSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	}
	else
	{
		SaveGame = Cast<UPlayerExperienceSaveGame>(UGameplayStatics::CreateSaveGameObject(UPlayerExperienceSaveGame::StaticClass()));
	}
	
#if WITH_EDITOR
	if (GEditor)
	{
		SaveGame->MarkEditorSave();
	}
#endif //WITH_EDITOR

	UGameplayStatics::AsyncSaveGameToSlot(SaveGame, SlotName, 0);
	SaveGame->MarkFromLoad();
	SaveGame->LoadedSlotName = SlotName;
	return SaveGame;
}

UPlayerExperienceSaveGame* UPlayerExperienceSaveGame::CreateRemoteAuthorityPlayerSaveData(ACorePlayerController* PlayerController)
{
	UPlayerExperienceSaveGame* SaveGame = Cast<UPlayerExperienceSaveGame>(UGameplayStatics::CreateSaveGameObject(UPlayerExperienceSaveGame::StaticClass()));
	SaveGame->MarkFromRPC();
	return SaveGame;
}

bool UPlayerExperienceSaveGame::ResetPlayerSaveData(ACorePlayerController* PlayerController)
{
	const FString SlotName = GetPlayerID(PlayerController);

	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		return false;
	}

	return UGameplayStatics::DeleteGameInSlot(SlotName, 0);
}

bool UPlayerExperienceSaveGame::RequestSave(UPlayerExperienceSaveGame* SaveGame)
{
	if (!SaveGame || SaveGame->GetSaveType() != ESaveGameType::FromLoad)
	{
		return false;
	}

	if (SaveGame->IsSaving())
	{
		SaveGame->bPendingSave = true;
		return true;
	}

	SaveGame->bPendingSave = false;
	UGameplayStatics::AsyncSaveGameToSlot(SaveGame, SaveGame->LoadedSlotName, 0, FAsyncSaveGameToSlotDelegate::CreateUObject(SaveGame, &UPlayerExperienceSaveGame::SaveCompleted));
	return true;
}

bool UPlayerExperienceSaveGame::ReceiveServerStatistics(const FPlayerStatisticsStruct& ServerStatistics)
{
	Statistics.ReplaceWithLarger(ServerStatistics);
	return true;
}

bool UPlayerExperienceSaveGame::ReceiveServerExperience(const FExperienceStruct& ServerExperience)
{
	Experience.ReplaceWithLarger(ServerExperience);
	return true;
}

void UPlayerExperienceSaveGame::SaveCompleted(const FString& SlotName, const int32 UserIndex, bool bSuccess)
{
	bIsSaving = false;

	if (bPendingSave)
	{
		RequestSave(this);
	}
}

bool UPlayerExperienceSaveGame::PushPlayerData(FExperienceStruct& InExperience, FPlayerStatisticsStruct& InStatistics, FPlayerSelectionStruct& InPlayerSelection)
{
	if (SaveGameType != ESaveGameType::FromRPC)
	{
		return false;
	}

	Experience = MoveTemp(InExperience);
	Statistics = MoveTemp(InStatistics);
	PlayerSelection = MoveTemp(InPlayerSelection);

	return true;
}

uint64 UPlayerExperienceSaveGame::GetPlayerStatisticsValue(EPlayerStatisticType StatisticsType) const
{
	return Statistics.Get(StatisticsType);
}

uint64 UPlayerExperienceSaveGame::SetPlayerStatisticsValue(EPlayerStatisticType StatisticsType, uint64 InValue)
{
	return Statistics.Set(StatisticsType, InValue);
}

uint64 UPlayerExperienceSaveGame::AddPlayerStatisticsValue(EPlayerStatisticType StatisticsType, uint64 Delta)
{
	return Statistics.Add(StatisticsType, Delta);
}

bool UPlayerExperienceSaveGame::CheckAndMarkStatusEffectReceived(TSubclassOf<UStatusEffectBase> StatusEffectClass)
{
	if (LocalData.HasReceivedStatusEffect(TSoftClassPtr<UStatusEffectBase>(StatusEffectClass)))
	{
		return true;
	}

	LocalData.MarkStatusEffectReceived(TSoftClassPtr<UStatusEffectBase>(StatusEffectClass));
	return false;
}