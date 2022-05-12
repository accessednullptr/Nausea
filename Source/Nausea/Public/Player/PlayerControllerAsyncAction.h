// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "PlayerControllerAsyncAction.generated.h"

class ACorePlayerController;

/**
 * 
 */
UCLASS()
class NAUSEA_API UPlayerControllerAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UBlueprintAsyncActionBase Interface
	virtual void Activate() override;
//~ End UBlueprintAsyncActionBase Interface

protected:
	//Generic failure state. Can be called via UPlayerControllerAsyncAction::Activate if WorldContextObject or OwningPlayerController is not valid.
	virtual void OnFailed();

protected:
	UPROPERTY()
	TWeakObjectPtr<ACorePlayerController> OwningPlayerController;

	UPROPERTY()
	bool bFailed = false;
};

class UPlayerClassComponent;
enum class EPlayerClassSelectionResponse : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPlayerClassSelectedResult, TSubclassOf<UPlayerClassComponent>, PlayerClass, EPlayerClassVariant, Variant, EPlayerClassSelectionResponse, Result);

UCLASS()
class USelectPlayerClassAsyncAction : public UPlayerControllerAsyncAction
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintAssignable)
	FOnPlayerClassSelectedResult OnSelectionResult;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = "Online")
	static USelectPlayerClassAsyncAction* SelectPlayerClass(ACorePlayerController* PlayerController, TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant);

	// UBlueprintAsyncActionBase interface
	virtual void Activate() override;
	// End of UBlueprintAsyncActionBase interface

protected:
	virtual void OnFailed() override;

	void OnComplete(EPlayerClassSelectionResponse Response);

private:
	UFUNCTION()
	void OnPlayerClassSelectionResult(ACorePlayerController* PlayerController, TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, EPlayerClassSelectionResponse RequestResponse, uint8 RequestID);

protected:
	UPROPERTY()
	TSubclassOf<UPlayerClassComponent> RequestedPlayerClass = nullptr;
	UPROPERTY()
	EPlayerClassVariant RequestedPlayerClassVariant;
	UPROPERTY()
	uint8 SelectionRequestID;
};

class UInventory;
enum class EInventorySelectionResponse : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnInventorySelectedResult, TSubclassOf<UPlayerClassComponent>, PlayerClass, EPlayerClassVariant, Variant, const TArray<TSubclassOf<UInventory>>&, Inventory, EInventorySelectionResponse, Result);

UCLASS()
class USelectInventoryAsyncAction : public UPlayerControllerAsyncAction
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintAssignable)
	FOnInventorySelectedResult OnSelectionResult;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = "Online")
	static USelectInventoryAsyncAction* SelectPlayerClassInventory(ACorePlayerController* PlayerController, TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, TArray<TSubclassOf<UInventory>> Inventory);

	// UBlueprintAsyncActionBase interface
	virtual void Activate() override;
	// End of UBlueprintAsyncActionBase interface

protected:
	virtual void OnFailed() override;

	void OnComplete(EInventorySelectionResponse Response);

private:
	UFUNCTION()
	void OnInventorySelectionResult(ACorePlayerController* PlayerController, TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, const TArray<TSubclassOf<UInventory>>& Inventory, EInventorySelectionResponse RequestResponse, uint8 RequestID);

protected:
	UPROPERTY()
	TSubclassOf<UPlayerClassComponent> RequestedPlayerClass = nullptr;
	UPROPERTY()
	EPlayerClassVariant RequestedVariant;
	UPROPERTY()
	TArray<TSubclassOf<UInventory>> RequestedInventory;
	UPROPERTY()
	uint8 SelectionRequestID;
};