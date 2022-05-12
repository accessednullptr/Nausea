// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#include "Player/PlayerControllerAsyncAction.h"
#include "NauseaHelpers.h"
#include "Player/NauseaPlayerController.h"
#include "Player/PlayerStatistics/PlayerStatisticsComponent.h"
#include "Weapon/Inventory.h"

UPlayerControllerAsyncAction::UPlayerControllerAsyncAction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UPlayerControllerAsyncAction::Activate()
{
	if (!OwningPlayerController.IsValid())
	{
		OnFailed();
		return;
	}
}

void UPlayerControllerAsyncAction::OnFailed()
{
	SetReadyToDestroy();
	bFailed = true;
}

USelectPlayerClassAsyncAction::USelectPlayerClassAsyncAction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RequestedPlayerClassVariant = EPlayerClassVariant::Invalid;
}

USelectPlayerClassAsyncAction* USelectPlayerClassAsyncAction::SelectPlayerClass(ACorePlayerController* PlayerController, TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant)
{
	USelectPlayerClassAsyncAction* Proxy = NewObject<USelectPlayerClassAsyncAction>();
	Proxy->OwningPlayerController = PlayerController;
	Proxy->RequestedPlayerClass = PlayerClass;
	Proxy->RequestedPlayerClassVariant = Variant;
	Proxy->RegisterWithGameInstance(PlayerController->GetWorld()->GetGameInstance());
	return Proxy;
}

void USelectPlayerClassAsyncAction::Activate()
{
	Super::Activate();

	if (bFailed)
	{
		return;
	}

	if (!RequestedPlayerClass)
	{
		OnFailed();
		return;
	}

	EPlayerClassSelectionResponse Result = OwningPlayerController->SelectPlayerClass(RequestedPlayerClass, RequestedPlayerClassVariant, SelectionRequestID);

	switch (Result)
	{
	case EPlayerClassSelectionResponse::Pending:
		OwningPlayerController->OnPlayerClassSelectionResponse.AddDynamic(this, &USelectPlayerClassAsyncAction::OnPlayerClassSelectionResult);
		return;
	}

	OnComplete(Result);
}

void USelectPlayerClassAsyncAction::OnFailed()
{
	OnSelectionResult.Broadcast(RequestedPlayerClass, RequestedPlayerClassVariant, EPlayerClassSelectionResponse::Invalid);
	Super::OnFailed();
}

void USelectPlayerClassAsyncAction::OnComplete(EPlayerClassSelectionResponse Response)
{
	OnSelectionResult.Broadcast(RequestedPlayerClass, RequestedPlayerClassVariant, Response);
	SetReadyToDestroy();
}

void USelectPlayerClassAsyncAction::OnPlayerClassSelectionResult(ACorePlayerController* PlayerController, TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, EPlayerClassSelectionResponse RequestResponse, uint8 RequestID)
{
	if (RequestID != SelectionRequestID)
	{
		return;
	}

	OnComplete(RequestResponse);
}

USelectInventoryAsyncAction::USelectInventoryAsyncAction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

USelectInventoryAsyncAction* USelectInventoryAsyncAction::SelectPlayerClassInventory(ACorePlayerController* PlayerController, TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, TArray<TSubclassOf<UInventory>> Inventory)
{
	USelectInventoryAsyncAction* Proxy = NewObject<USelectInventoryAsyncAction>();
	Proxy->OwningPlayerController = PlayerController;
	Proxy->RequestedPlayerClass = PlayerClass;
	Proxy->RequestedVariant = Variant;
	Proxy->RequestedInventory = Inventory;
	Proxy->RegisterWithGameInstance(PlayerController->GetWorld()->GetGameInstance());
	return Proxy;
}

void USelectInventoryAsyncAction::Activate()
{
	Super::Activate();

	if (bFailed)
	{
		return;
	}

	if (!RequestedPlayerClass || !OwningPlayerController->GetPlayerStatisticsComponent())
	{
		OnFailed();
		return;
	}

	EInventorySelectionResponse Result = OwningPlayerController->GetPlayerStatisticsComponent()->SetInventorySelection(RequestedPlayerClass, RequestedVariant, RequestedInventory, SelectionRequestID);

	switch (Result)
	{
	case EInventorySelectionResponse::Pending:
		OwningPlayerController->GetPlayerStatisticsComponent()->OnInventorySelectionResponse.AddDynamic(this, &USelectInventoryAsyncAction::OnInventorySelectionResult);
		return;
	}
}

void USelectInventoryAsyncAction::OnFailed()
{
	OnSelectionResult.Broadcast(RequestedPlayerClass, RequestedVariant, RequestedInventory, EInventorySelectionResponse::Invalid);
	Super::OnFailed();
}

void USelectInventoryAsyncAction::OnComplete(EInventorySelectionResponse Response)
{
	OnSelectionResult.Broadcast(RequestedPlayerClass, RequestedVariant, FNauseaHelpers::ConvertFromSoftClass(OwningPlayerController->GetPlayerStatisticsComponent()->GetInventorySelection(RequestedPlayerClass, RequestedVariant)), Response);
	SetReadyToDestroy();
}

void USelectInventoryAsyncAction::OnInventorySelectionResult(ACorePlayerController* PlayerController, TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, const TArray<TSubclassOf<UInventory>>& Inventory, EInventorySelectionResponse RequestResponse, uint8 RequestID)
{
	if (RequestID != SelectionRequestID)
	{
		return;
	}
}