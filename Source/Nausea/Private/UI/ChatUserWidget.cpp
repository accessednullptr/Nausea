// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "UI/ChatUserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Player/CorePlayerController.h"
#include "Player/CoreHUD.h"

UChatUserWidget::UChatUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

bool UChatUserWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	if (ACorePlayerController* CorePlayerController = GetOwningCorePlayerController())
	{
		if (!CorePlayerController->OnReceivedLocalMessage.IsAlreadyBound(this, &UChatUserWidget::ProcessLocalMessage))
		{
			CorePlayerController->OnReceivedLocalMessage.AddDynamic(this, &UChatUserWidget::ProcessLocalMessage);
		}
	}

	return true;
}

void UChatUserWidget::ProcessLocalMessage(const FName& MessageType, int32 MessageIndex, const FString& MessageString, ACorePlayerState* PlayerStateA, ACorePlayerState* PlayerStateB, const UObject* OptionalObject)
{
	//Ignore Event Type messages.
	if (MessageType == FName("Event"))
	{
		return;
	}

	UChatEntryUserWidget* ChatEntry = GetChatEntryInstance();

	EChatEntryType EntryType = EChatEntryType::Notification;
	if (MessageType == NAME_Say)
	{
		EntryType = EChatEntryType::Text;
	}
	else if (MessageType == NAME_VoiceCommand)
	{
		EntryType = EChatEntryType::VoiceCommand;
	}
	else if (MessageType == NAME_Notification)
	{
		EntryType = EChatEntryType::Notification;
	}
	else if (MessageType == NAME_NotificationCritical)
	{
		EntryType = EChatEntryType::NotificationCritical;
	}

	ChatEntry->SetEntryData(PlayerStateA, EntryType, MessageString);
	AddChatEntry(ChatEntry);
}

UChatEntryUserWidget* UChatUserWidget::GetChatEntryInstance()
{
	if (!WidgetTree || !ChatEntryWidgetClass)
	{
		return nullptr;
	}

	if (ACoreHUD* CoreHUD = GetOwningPlayer() ? GetOwningPlayer()->GetHUD<ACoreHUD>() : nullptr)
	{
		if (UChatEntryUserWidget* ChatEntry = CoreHUD->GetWidgetFromPool<UChatEntryUserWidget>(ChatEntryWidgetClass))
		{
			return ChatEntry;
		}
	}

	return WidgetTree->ConstructWidget<UChatEntryUserWidget>(ChatEntryWidgetClass);
}

UChatEntryUserWidget::UChatEntryUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UChatEntryUserWidget::SetEntryData(ACorePlayerState* InCorePlayerState, EChatEntryType InType, const FString& InText)
{
	CorePlayerState = InCorePlayerState;
	Type = InType;
	Text = InText;

	ReceiveChatEntryData(InCorePlayerState, InType, InText);
}