// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "UI/CoreUserWidget.h"
#include "Player/PlayerPrompt/PlayerPromptTypes.h"
#include "PromptUserWidget.generated.h"

class UPromptInfo;

/**
 * 
 */
UCLASS()
class NAUSEA_API UPromptUserWidget : public UCoreUserWidget
{
	GENERATED_UCLASS_BODY()
	
public:
	DECLARE_EVENT_TwoParams(UPromptUserWidget, FOnPromptAccepted, const FPromptHandle&, bool)
	FOnPromptAccepted OnPromptResponseEvent;

	void SetPromptData(const struct FPromptData& Prompt);
	
	//Prompt has been somehow dismissed. Will call UPromptUserWidget::OnPromptResponse with bAccepted set to false.
	void DismissPrompt();

protected:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic)
	void OnReceivePrompt(TSubclassOf<UPromptInfo> PromptInfo);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic)
	void OnPromptResponse(bool bAccepted);

	UFUNCTION(BlueprintCallable, BlueprintCosmetic)
	void AcceptPrompt();
	UFUNCTION(BlueprintCallable, BlueprintCosmetic)
	void DeclinePrompt();

	void SendPromptResponse(bool bAccepted);

private:
	UPROPERTY(Transient)
	bool bResponded = false;
	UPROPERTY(Transient)
	FPromptHandle OwningPromptHandle;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCreatePromptUserWidgetResponse, UPromptUserWidget*, PromptUserWidget);

UCLASS()
class NAUSEA_API UCreatePromptUserWidgetAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UBlueprintAsyncActionBase Interface
	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;
//~ End UBlueprintAsyncActionBase Interface

public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = UI)
	static UCreatePromptUserWidgetAsyncAction* CreatePromptUserWidget(ACorePlayerController* OwningPlayer, const FPromptHandle& PromptHandle);

public:
	UPROPERTY(BlueprintAssignable, Category = PromptInfo)
	FOnCreatePromptUserWidgetResponse OnCreatePromptUserWidgetResponse;

protected:
	UFUNCTION()
	void OnWidgetClassLoadComplete();

	UFUNCTION()
	void OnFailed();

protected:
	UPROPERTY()
	TWeakObjectPtr<ACorePlayerController> OwningPlayerController = nullptr;

	UPROPERTY()
	TSoftClassPtr<UPromptUserWidget> SoftWidgetClass = nullptr;

	TSharedPtr<FStreamableHandle> StreamableHandle;

	FPromptHandle PromptHandle = FPromptHandle::InvalidHandle;

	UPROPERTY()
	bool bFailed = false;
};