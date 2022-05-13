// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "UI/CoreUserWidget.h"
#include "Nausea.h"
#include "Player/CorePlayerController.h"
#include "Player/CoreHUD.h"
#include "Player/PlayerStatistics/PlayerStatisticsComponent.h"
#include "Player/CorePlayerState.h"
#include "Character/CoreCharacter.h"
#include "UI/NauseaWidgetComponent.h"

UCoreUserWidget::UCoreUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

bool UCoreUserWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	if (!GetPlayerContext().IsValid() || !GetPlayerContext().IsInitialized())
	{
		return true;
	}

	if (!ensure(GetOwningCorePlayerController()))
	{
		return true;
	}

	if (bNotifyOnPlayerDataReady && GetOwningCorePlayerController()->GetPlayerStatisticsComponent())
	{
		GetOwningCorePlayerController()->GetPlayerStatisticsComponent()->OnPlayerDataReady.AddDynamic(this, &UCoreUserWidget::ReceivedPlayerDataReady);
	}

	if (bNotifyOnReceivePlayerState)
	{
		if (ACorePlayerState* CorePlayerState = GetOwningCorePlayerController()->GetPlayerState<ACorePlayerState>())
		{
			ReceivedPlayerState(GetOwningCorePlayerController(), CorePlayerState);
		}
		else if (!GetOwningCorePlayerController()->OnReceivedPlayerState.IsAlreadyBound(this, &UCoreUserWidget::ReceivedPlayerState))
		{
			GetOwningCorePlayerController()->OnReceivedPlayerState.AddDynamic(this, &UCoreUserWidget::ReceivedPlayerState);
		}
	}

	return true;
}

void UCoreUserWidget::InitializeWidgetComponent(UNauseaWidgetComponent* OwningComponent)
{
	OwningWidgetComponent = OwningComponent;
	OnReceivedWidgetComponent(OwningWidgetComponent);
}

UNauseaWidgetComponent* UCoreUserWidget::GetWidgetComponent() const
{
	return OwningWidgetComponent;
}

bool UCoreUserWidget::IsWidgetComponentWidget() const
{
	return OwningWidgetComponent != nullptr;
}

AActor* UCoreUserWidget::GetWidgetComponentOwner() const
{
	return OwningWidgetComponent ? OwningWidgetComponent->GetOwner() : nullptr;
}

ACorePlayerController* UCoreUserWidget::GetOwningCorePlayerController() const
{
	return GetOwningPlayer<ACorePlayerController>();
}

ACoreCharacter* UCoreUserWidget::GetOwningPlayerCharacter() const
{
	return GetOwningPlayerPawn<ACoreCharacter>();
}

void UCoreUserWidget::ReleaseWidgetToPool()
{
	if (ACoreHUD* CoreHUD = GetOwningPlayer() ? GetOwningPlayer()->GetHUD<ACoreHUD>() : nullptr)
	{
		CoreHUD->ReleaseWidgetToPool(this);
		OnWidgetReleasedToPool();
	}

	RemoveFromParent();
}

void UCoreUserWidget::ReceivedPlayerDataReady()
{
	OnPlayerDataReady(GetOwningCorePlayerController(), GetOwningCorePlayerController()->GetPlayerStatisticsComponent());
}

void UCoreUserWidget::ReceivedPlayerState(ACorePlayerController* PlayerController, ACorePlayerState* PlayerState)
{
	OnReceivedPlayerState(PlayerState);

	if (bNotifyOnPlayerClassChanged && PlayerState)
	{
		if (!PlayerState->OnPlayerClassChanged.IsAlreadyBound(this, &UCoreUserWidget::ReceivedPlayerClassChange))
		{
			PlayerState->OnPlayerClassChanged.AddDynamic(this, &UCoreUserWidget::ReceivedPlayerClassChange);
		}

		if (PlayerState->GetPlayerClassComponent())
		{
			ReceivedPlayerClassChange(PlayerState, PlayerState->GetPlayerClassComponent());
		}
	}
}

void UCoreUserWidget::ReceivedPlayerClassChange(ACorePlayerState* PlayerState, UPlayerClassComponent* PlayerClass)
{
	OnReceivedPlayerClass(PlayerClass);
}

void UCoreUserWidget::K2_ReleaseWidgetToPool(UObject* Object)
{
	if (!Object || Object != this)
	{
		UE_LOG(LogUI, Error, TEXT("May not release widget %s from context of %s. Must be called within the widget."), *GetFullName(), Object ? *Object->GetFullName() : *FString("NULL"));
		
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		FText ErrorMessage = FText::Format(NSLOCTEXT("CoreUserWidget", "ExternalCoreUserWidgetRelease", "May not release widget {0} from context of {1}. Must be called within the widget."),
			FText::FromString(GetFullName()), FText::FromString(Object ? *Object->GetFullName() : *FString("NULL")));
		FMessageLog("PIE").Error(ErrorMessage);
#endif
		return;
	}
	
	ReleaseWidgetToPool();
}

void UCoreUserWidget::K2_SetWidgetMinimumDesiredSize(const FVector2D& InMinimumDesiredSize)
{
	SetMinimumDesiredSize(InMinimumDesiredSize);
}

UCoreWidgetAsyncAction::UCoreWidgetAsyncAction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UCoreWidgetAsyncAction::Activate()
{
	if (!OwningObject.IsValid() || SoftWidgetClass.IsNull())
	{
		bFailed = true;
		SetReadyToDestroy();
		return;
	}

	//If already loaded, don't bother with the streamable manager.
	if (SoftWidgetClass.Get())
	{
		OnCoreWidgetLoaded.Broadcast(OwningObject.Get(), SoftWidgetClass.Get());
		SetReadyToDestroy();
		return;
	}

	UAssetManager& AssetManager = UAssetManager::Get();

	if (!AssetManager.IsValid())
	{
		bFailed = true;
		SetReadyToDestroy();
		return;
	}

	FStreamableManager& StreamableManager = AssetManager.GetStreamableManager();
	StreamableHandle = StreamableManager.RequestAsyncLoad(SoftWidgetClass.ToSoftObjectPath());

	if (!StreamableHandle.IsValid())
	{
		bFailed = true;
		SetReadyToDestroy();
		return;
	}

	TWeakObjectPtr<UCoreWidgetAsyncAction> WeakThis = TWeakObjectPtr<UCoreWidgetAsyncAction>(this);
	auto StreamClassCompleteDelegate = [WeakThis] {
		if (!WeakThis.IsValid())
		{
			return;
		}

		if (!WeakThis->SoftWidgetClass.Get())
		{
			WeakThis->bFailed = true;
			WeakThis->SetReadyToDestroy();
		}

		if (!WeakThis->OwningObject.IsValid())
		{
			WeakThis->bFailed = true;
			WeakThis->SetReadyToDestroy();
			return;
		}

		WeakThis->OnCoreWidgetLoaded.Broadcast(WeakThis->OwningObject.Get(), WeakThis->SoftWidgetClass.Get());
		WeakThis->SetReadyToDestroy();
	};

	if (StreamableHandle->HasLoadCompleted())
	{
		StreamClassCompleteDelegate();
		return;
	}

	StreamableHandle->BindCompleteDelegate(FStreamableDelegate::CreateWeakLambda(this, StreamClassCompleteDelegate));
}

void UCoreWidgetAsyncAction::SetReadyToDestroy()
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

UCoreWidgetAsyncAction* UCoreWidgetAsyncAction::LoadCoreWidget(UObject* Object, TSoftClassPtr<UCoreUserWidget> WidgetClass)
{
	if (!Object || !Object->GetWorld())
	{
		return nullptr;
	}

	UCoreWidgetAsyncAction* Action = NewObject<UCoreWidgetAsyncAction>();
	Action->OwningObject = Object;
	Action->SoftWidgetClass = WidgetClass;
	Action->RegisterWithGameInstance(Object->GetWorld()->GetGameInstance());
	return Action;
}