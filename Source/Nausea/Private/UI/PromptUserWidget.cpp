// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "UI/PromptUserWidget.h"
#include "Player/CorePlayerController.h"
#include "Player/PlayerPromptComponent.h"

UPromptUserWidget::UPromptUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UPromptUserWidget::SetPromptData(const FPromptData& Prompt)
{
	OwningPromptHandle = Prompt.GetPromptHandle();
	OnReceivePrompt(Prompt.GetPromptInfoClass());
}

void UPromptUserWidget::DismissPrompt()
{
	SendPromptResponse(false);
}

void UPromptUserWidget::AcceptPrompt()
{
	SendPromptResponse(true);
}

void UPromptUserWidget::DeclinePrompt()
{
	SendPromptResponse(false);
}

void UPromptUserWidget::SendPromptResponse(bool bAccepted)
{
	if (bResponded)
	{
		return;
	}

	bResponded = true;
	OnPromptResponse(bAccepted);
	OnPromptResponseEvent.Broadcast(OwningPromptHandle, bAccepted);
}

UCreatePromptUserWidgetAsyncAction::UCreatePromptUserWidgetAsyncAction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UCreatePromptUserWidgetAsyncAction::Activate()
{
	if (!OwningPlayerController.IsValid() || SoftWidgetClass.IsNull() || !PromptHandle.IsValid())
	{
		OnFailed();
		return;
	}

	//If already loaded, don't bother with the streamable manager.
	if (SoftWidgetClass.Get())
	{
		OnWidgetClassLoadComplete();
		return;
	}

	UAssetManager& AssetManager = UAssetManager::Get();

	if (!AssetManager.IsValid())
	{
		OnFailed();
		return;
	}

	FStreamableManager& StreamableManager = AssetManager.GetStreamableManager();
	StreamableHandle = StreamableManager.RequestAsyncLoad(SoftWidgetClass.ToSoftObjectPath());

	if (!StreamableHandle.IsValid())
	{
		OnFailed();
		return;
	}

	TWeakObjectPtr<UCreatePromptUserWidgetAsyncAction> WeakThis = TWeakObjectPtr<UCreatePromptUserWidgetAsyncAction>(this);

	auto StreamClassCompleteDelegate = [WeakThis] {
		if (!WeakThis.IsValid())
		{
			return;
		}

		if (!WeakThis->SoftWidgetClass.Get())
		{
			WeakThis->OnFailed();
			return;
		}

		WeakThis->OnWidgetClassLoadComplete();
	};

	if (StreamableHandle->HasLoadCompleted())
	{
		StreamClassCompleteDelegate();
		return;
	}

	StreamableHandle->BindCompleteDelegate(FStreamableDelegate::CreateWeakLambda(this, StreamClassCompleteDelegate));
}

void UCreatePromptUserWidgetAsyncAction::SetReadyToDestroy()
{
	if (StreamableHandle.IsValid())
	{
		if (StreamableHandle->IsLoadingInProgress())
		{
			StreamableHandle->CancelHandle();
		}

		StreamableHandle.Reset();
	}

	Super::SetReadyToDestroy();
}

UCreatePromptUserWidgetAsyncAction* UCreatePromptUserWidgetAsyncAction::CreatePromptUserWidget(ACorePlayerController* OwningPlayer, const FPromptHandle& PromptHandle)
{
	if (!OwningPlayer || !OwningPlayer->GetPlayerPromptComponent())
	{
		return nullptr;
	}

	const FPromptData& PromptData = OwningPlayer->GetPlayerPromptComponent()->GetPromptData(PromptHandle);

	if (!PromptData.IsValid())
	{
		return nullptr;
	}

	const UPromptInfo* PromptInfo = PromptData.GetPromptInfo();

	if (!PromptInfo)
	{
		return nullptr;
	}

	const TSoftClassPtr<UPromptUserWidget>& PromptUserWidgetClass = PromptInfo->GetUserWidgetClass();

	if (PromptUserWidgetClass.IsNull())
	{
		return nullptr;
	}

	UCreatePromptUserWidgetAsyncAction* Action = NewObject<UCreatePromptUserWidgetAsyncAction>();
	Action->OwningPlayerController = OwningPlayer;
	Action->SoftWidgetClass = PromptUserWidgetClass;
	Action->PromptHandle = PromptHandle;
	Action->RegisterWithGameInstance(OwningPlayer->GetWorld()->GetGameInstance());
	return Action;
}

void UCreatePromptUserWidgetAsyncAction::OnWidgetClassLoadComplete()
{
	if (!OwningPlayerController.IsValid() || !OwningPlayerController->GetPlayerPromptComponent())
	{
		OnFailed();
		return;
	}

	UPromptUserWidget* PromptUserWidget = CreateWidget<UPromptUserWidget>(OwningPlayerController.Get(), SoftWidgetClass.Get());

	if (!PromptUserWidget)
	{
		OnFailed();
		return;
	}

	OwningPlayerController->GetPlayerPromptComponent()->AddPromptUserWidget(PromptHandle, PromptUserWidget);
	OnCreatePromptUserWidgetResponse.Broadcast(PromptUserWidget);
	SetReadyToDestroy();
}

void UCreatePromptUserWidgetAsyncAction::OnFailed()
{
	bFailed = true;
	SetReadyToDestroy();
}