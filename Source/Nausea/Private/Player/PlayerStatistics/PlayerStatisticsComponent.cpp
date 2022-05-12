// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerStatistics/PlayerStatisticsComponent.h"
#include "NauseaHelpers.h"
#include "Player/PlayerStatistics/PlayerExperienceSaveGame.h"
#include "System/CoreGameState.h"
#include "Player/CorePlayerController.h"
#include "Player/PlayerClassComponent.h"

DECLARE_CYCLE_STAT(TEXT("Push Player Statistics Data"),
	STAT_FSimpleDelegateGraphTask_PushingPlayerStatisticsData,
	STATGROUP_TaskGraphTasks);

UPlayerStatisticsComponent::UPlayerStatisticsComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

ACorePlayerController* UPlayerStatisticsComponent::GetOwningPlayerController() const
{
	return Cast<ACorePlayerController>(GetOwner());
}

bool UPlayerStatisticsComponent::IsLocalPlayerController() const
{
	ACorePlayerController* CorePlayerController = GetOwningPlayerController();

	if (!CorePlayerController)
	{
		return false;
	}

	return CorePlayerController->IsLocalPlayerController();
}

void UPlayerStatisticsComponent::LoadPlayerData()
{
	if (!ensure(IsLocalPlayerController()))
	{
		return;
	}

	PlayerData = UPlayerExperienceSaveGame::LoadPlayerSaveData(GetOwningPlayerController());

	if (!PlayerData)
	{
		return;
	}

	ValidateInitialInventorySelection();
	SendPlayerData();
}

void UPlayerStatisticsComponent::ResetPlayerData()
{
	if (!GetOwningPlayerController() || !GetOwningPlayerController()->IsNetMode(NM_Standalone))
	{
		return;
	}

	UPlayerExperienceSaveGame::ResetPlayerSaveData(GetOwningPlayerController());
	
	bPlayerStatsReady = false;

	PlayerData = UPlayerExperienceSaveGame::LoadPlayerSaveData(GetOwningPlayerController());
	
	ValidateInitialInventorySelection();
	SendPlayerData();
}

void UPlayerStatisticsComponent::UpdateClientPlayerData()
{
	if (!GetOwningPlayerController() || IsLocalPlayerController() || !GetPlayerData()
		|| GetOwningPlayerController()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	Client_Reliable_UpdatePlayerData(GetPlayerData()->GetExperienceData(), GetPlayerData()->GetPlayerStatisticsData());
}

void UPlayerStatisticsComponent::SendPlayerData()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		Server_Reliable_SendPlayerData(PlayerData->GetExperienceData(), PlayerData->GetPlayerStatisticsData(), PlayerData->GetPlayerSelectionData());
		return;
	}

	if (IsPlayerStatsReady())
	{
		return;
	}

	PlayerDataReady();
}

void UPlayerStatisticsComponent::ReceivePlayerData(FExperienceStruct& Experience, FPlayerStatisticsStruct& Statistics, FPlayerSelectionStruct& PlayerSelection)
{
	if (IsPlayerStatsReady())
	{
		return;
	}

	PlayerData = UPlayerExperienceSaveGame::CreateRemoteAuthorityPlayerSaveData(GetOwningPlayerController());
	PlayerData->PushPlayerData(Experience, Statistics, PlayerSelection);
	ValidateInitialInventorySelection();
	PlayerDataReady();
}

void UPlayerStatisticsComponent::PlayerDataReady()
{
	bPlayerStatsReady = true;

	if (GetPlayerData())
	{
		GetPlayerData()->OnSelectedPlayerClassChanged.AddUObject(this, &UPlayerStatisticsComponent::OnPlayerClassSelectionChanged);
		GetPlayerData()->OnSelectedPlayerClassVariantChanged.AddUObject(this, &UPlayerStatisticsComponent::OnPlayerClassVariantSelectionChanged);
	}

	OnPlayerDataReady.Broadcast();

	if (!IsLocalPlayerController() && GetOwnerRole() == ROLE_Authority)
	{
		Client_Reliable_SendPlayerDataAck(true);
	}
}

bool UPlayerStatisticsComponent::Server_Reliable_SendPlayerData_Validate(FExperienceStruct Experience, FPlayerStatisticsStruct Statistics, FPlayerSelectionStruct PlayerSelection)
{
	return true;
}

void UPlayerStatisticsComponent::Server_Reliable_SendPlayerData_Implementation(FExperienceStruct Experience, FPlayerStatisticsStruct Statistics, FPlayerSelectionStruct PlayerSelection)
{
	if (IsPlayerStatsReady())
	{
		return;
	}

	ReceivePlayerData(Experience, Statistics, PlayerSelection);
}

void UPlayerStatisticsComponent::Client_Reliable_SendPlayerDataAck_Implementation(bool bSuccess)
{
	if (!bSuccess)
	{
		SendPlayerData();
		return;
	}

	PlayerDataReady();
}

void UPlayerStatisticsComponent::Client_Reliable_UpdatePlayerData_Implementation(FExperienceStruct Experience, FPlayerStatisticsStruct Statistics)
{
	if (!GetPlayerData())
	{
		return;
	}

	GetPlayerData()->ReceiveServerStatistics(Statistics);
	GetPlayerData()->ReceiveServerExperience(Experience);
	UPlayerExperienceSaveGame::RequestSave(GetPlayerData());
}

TSoftClassPtr<UPlayerClassComponent> UPlayerStatisticsComponent::GetSelectedPlayerClass() const
{
	if (!GetPlayerData())
	{
		return nullptr;
	}

	return GetPlayerData()->GetPlayerSelectionData().GetSelectedPlayerClass();
}

EPlayerClassVariant UPlayerStatisticsComponent::GetSelectedPlayerClassVariant(TSoftClassPtr<UPlayerClassComponent> PlayerClass) const
{
	if (!GetPlayerData())
	{
		return EPlayerClassVariant::Invalid;
	}

	if (!GetPlayerData()->GetPlayerSelectionData().GetPlayerClassVariantSelection().Contains(PlayerClass))
	{
		return EPlayerClassVariant::Primary;
	}

	return GetPlayerData()->GetPlayerSelectionData().GetPlayerClassVariantSelection()[PlayerClass];
}

bool UPlayerStatisticsComponent::SetPlayerClassSelection(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant)
{
	if (!GetPlayerData())
	{
		return false;
	}

	if (GetPlayerData()->UpdatePlayerClassSelection(PlayerClass, Variant))
	{
		RequestSave();
		return true;
	}

	return false;
}

bool UPlayerStatisticsComponent::SetPlayerClassVariantSelection(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant)
{
	if (!GetPlayerData())
	{
		return false;
	}

	if (GetPlayerData()->UpdatePlayerClassVariantSelection(PlayerClass, Variant))
	{
		RequestSave();
		return true;
	}

	return false;
}

EInventorySelectionResponse UPlayerStatisticsComponent::SetInventorySelection(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, const TArray<TSubclassOf<UInventory>>& InInventorySelection)
{
	uint8 RequestID;
	return SetInventorySelection(PlayerClass, Variant, InInventorySelection, RequestID);
}

static uint8 SelectInventoryRequestID = 0;
EInventorySelectionResponse UPlayerStatisticsComponent::SetInventorySelection(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, const TArray<TSubclassOf<UInventory>>& InInventorySelection, uint8& RequestID)
{
	if (!GetPlayerData())
	{
		return EInventorySelectionResponse::Invalid;
	}

	if (!GetPlayerData()->UpdatePlayerInventorySelectionData(PlayerClass, Variant, InInventorySelection))
	{
		return EInventorySelectionResponse::AlreadySelected;
	}
	else if(IsLocalPlayerController())
	{
		RequestSave();
	}

	if (IsLocalPlayerController() && GetOwnerRole() != ROLE_Authority)
	{
		SelectInventoryRequestID++;
		RequestID = SelectInventoryRequestID;
		Server_Reliable_UpdateInventorySelection(PlayerClass, Variant, FNauseaHelpers::ConvertFromSoftClass(GetInventorySelection(PlayerClass, Variant)), RequestID);
		return EInventorySelectionResponse::Pending;
	}

	return EInventorySelectionResponse::Success;
}

const TArray<TSoftClassPtr<UInventory>>& UPlayerStatisticsComponent::GetInventorySelection(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant)
{
	if (!GetPlayerData())
	{
		return FInventorySelectionEntry::InvalidConfigArray;
	}

	return GetPlayerData()->GetPlayerInventorySelection(PlayerClass, Variant);
}

bool UPlayerStatisticsComponent::Server_Reliable_UpdateInventorySelection_Validate(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, const TArray<TSubclassOf<UInventory>>& InInventorySelection, uint8 RequestID)
{
	return true;
}

void UPlayerStatisticsComponent::Server_Reliable_UpdateInventorySelection_Implementation(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, const TArray<TSubclassOf<UInventory>>& InInventorySelection, uint8 RequestID)
{
	if (!GetPlayerData())
	{
		return;
	}

	SetInventorySelection(PlayerClass, Variant, InInventorySelection);
}

void UPlayerStatisticsComponent::Client_Reliable_UpdateInventorySelectionResponse_Implementation(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, const TArray<TSubclassOf<UInventory>>& ResultInventorySelection, EInventorySelectionResponse Response, uint8 RequestID)
{
	switch (Response)
	{
	case EInventorySelectionResponse::SuccessWithChanges:
		GetPlayerData()->UpdatePlayerInventorySelectionData(PlayerClass, Variant, ResultInventorySelection);
		break;
	}

	OnInventorySelectionResponse.Broadcast(GetOwningPlayerController(), PlayerClass, Variant, ResultInventorySelection, Response, RequestID);
}

bool UPlayerStatisticsComponent::ValidateInitialInventorySelection()
{
	if (!GetPlayerData())
	{
		return true;
	}

	return true;
}

bool UPlayerStatisticsComponent::RequestSave()
{
	if (!GetPlayerData())
	{
		return false;
	}

	if (GetPlayerData()->GetSaveType() == ESaveGameType::FromLoad)
	{
		return UPlayerExperienceSaveGame::RequestSave(GetPlayerData());
	}
	else if(GetPlayerData()->GetSaveType() == ESaveGameType::FromRPC)
	{
		UpdateClientPlayerData();
	}

	return false;
}

void UPlayerStatisticsComponent::OnPlayerClassSelectionChanged(TSubclassOf<UPlayerClassComponent> PlayerClass)
{
	GetOwningPlayerController()->OnPlayerClassSelectionChange(PlayerClass);
}

void UPlayerStatisticsComponent::OnPlayerClassVariantSelectionChanged(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant)
{
	GetOwningPlayerController()->OnPlayerClassVariantSelectionChange(PlayerClass, Variant);
}

void UPlayerStatisticsComponent::UpdatePlayerStatistic(EPlayerStatisticType StatisticType, float Delta)
{
	PlayerStatisticsMapCache.FindOrAdd(StatisticType) += Delta;

	if (PlayerStatisticsMapCache[StatisticType] > 1.f && !bPendingStatPush)
	{
		bPendingStatPush = true;

		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
			FSimpleDelegateGraphTask::FDelegate::CreateUObject(this, &UPlayerStatisticsComponent::PushPlayerStatisticCache),
			GET_STATID(STAT_FSimpleDelegateGraphTask_PushingPlayerStatisticsData), NULL, ENamedThreads::GameThread);
	}
}

void UPlayerStatisticsComponent::PushPlayerStatisticCache()
{
	TArray<EPlayerStatisticType> KeyList;
	PlayerStatisticsMapCache.GenerateKeyArray(KeyList);

	UPlayerExperienceSaveGame* CurrentPlayerData = GetPlayerData();

	if (!CurrentPlayerData)
	{
		bPendingStatPush = false;
		return;
	}

	for (EPlayerStatisticType Key : KeyList)
	{
		const float TruncatedFloatValue = FMath::TruncToFloat(PlayerStatisticsMapCache[Key]);
		PlayerStatisticsMapCache[Key] -= TruncatedFloatValue;
		
		const uint64 ValueAdded = uint64(TruncatedFloatValue);
		CurrentPlayerData->AddPlayerStatisticsValue(Key, ValueAdded);
		
		OnPlayerStatisticsUpdate.Broadcast(this, Key, ValueAdded);
	}

	bPendingStatPush = false;
}

void UPlayerStatisticsComponent::UpdatePlayerExperience(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, uint32 Delta)
{
	UPlayerExperienceSaveGame* CurrentPlayerData = GetPlayerData();

	if (!CurrentPlayerData)
	{
		return;
	}

	CurrentPlayerData->AddExperienceValue(TSoftClassPtr<UPlayerClassComponent>(PlayerClass), Variant, Delta);
	RequestSave();
	OnPlayerExperienceUpdate.Broadcast(this, PlayerClass, Variant, Delta);
}

uint64 UPlayerStatisticsComponent::GetPlayerClassExperience(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant)
{
	UPlayerExperienceSaveGame* CurrentPlayerData = GetPlayerData();

	if (!CurrentPlayerData)
	{
		return 0;
	}

	return CurrentPlayerData->GetExperienceValue(TSoftClassPtr<UPlayerClassComponent>(PlayerClass), Variant);
}

bool UPlayerStatisticsComponent::CheckAndMarkStatusEffectReceived(TSubclassOf<UStatusEffectBase> StatusEffectClass)
{
	UPlayerExperienceSaveGame* CurrentPlayerData = GetPlayerData();

	if (!CurrentPlayerData)
	{
		return false;
	}

	return CurrentPlayerData->CheckAndMarkStatusEffectReceived(StatusEffectClass);
}