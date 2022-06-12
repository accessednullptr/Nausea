// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Player/CoreHUD.h"
#include "Player/CorePlayerController.h"
#include "Player/PlayerStatistics/PlayerStatisticsComponent.h"
#include "Player/CorePlayerState.h"
#include "UI/CoreUserWidget.h"
#include "Player/PlayerPromptComponent.h"

ACoreHUD::ACoreHUD(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void ACoreHUD::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (!GetOwningCorePlayerController())
	{
		return;
	}

	if (GetOwningCorePlayerController()->GetPlayerStatisticsComponent())
	{
		if (!GetOwningCorePlayerController()->GetPlayerStatisticsComponent()->IsPlayerStatsReady())
		{
			GetOwningCorePlayerController()->GetPlayerStatisticsComponent()->OnPlayerDataReady.AddDynamic(this, &ACoreHUD::ReceivedPlayerDataReady);
		}
		else
		{
			ReceivedPlayerDataReady();
		}
	}

	if (GetOwningCorePlayerController()->GetPlayerPromptComponent())
	{
		GetOwningCorePlayerController()->GetPlayerPromptComponent()->OnRequestDisplayPrompt.AddDynamic(this, &ACoreHUD::ReceivedPlayerPrompt);
	}

	if (ACorePlayerState* CorePlayerState = GetOwningCorePlayerController()->GetPlayerState<ACorePlayerState>())
	{
		ReceivedPlayerState(GetOwningCorePlayerController(), CorePlayerState);
	}
	else if (!GetOwningCorePlayerController()->OnReceivedPlayerState.IsAlreadyBound(this, &ACoreHUD::ReceivedPlayerState))
	{
		GetOwningCorePlayerController()->OnReceivedPlayerState.AddDynamic(this, &ACoreHUD::ReceivedPlayerState);
	}
}

ACorePlayerController* ACoreHUD::GetOwningCorePlayerController() const
{
	return Cast<ACorePlayerController>(PlayerOwner);
}

void ACoreHUD::ReleaseWidgetToPool(UWidget* Widget)
{
	if (!Widget)
	{
		return;
	}

	WidgetPool.FindOrAdd(Widget->GetClass()).PushWidget(Widget);
}

void ACoreHUD::ReceivedPlayerDataReady()
{
	OnPlayerDataReady(GetOwningCorePlayerController(), GetOwningCorePlayerController()->GetPlayerStatisticsComponent());

	if (GetOwningCorePlayerController()->GetPlayerStatisticsComponent()->OnPlayerDataReady.IsAlreadyBound(this, &ACoreHUD::ReceivedPlayerDataReady))
	{
		GetOwningCorePlayerController()->GetPlayerStatisticsComponent()->OnPlayerDataReady.RemoveDynamic(this, &ACoreHUD::ReceivedPlayerDataReady);
	}
}

void ACoreHUD::ReceivedPlayerState(ACorePlayerController* PlayerController, ACorePlayerState* PlayerState)
{
	OnReceivedPlayerState(GetOwningCorePlayerController() ? GetOwningCorePlayerController()->GetPlayerState<ACorePlayerState>() : nullptr);

	if (PlayerState)
	{
		if (!PlayerState->OnPlayerClassChanged.IsAlreadyBound(this, &ACoreHUD::ReceivedPlayerClassChange))
		{
			PlayerState->OnPlayerClassChanged.AddDynamic(this, &ACoreHUD::ReceivedPlayerClassChange);
		}

		if (PlayerState->GetPlayerClassComponent())
		{
			ReceivedPlayerClassChange(PlayerState, PlayerState->GetPlayerClassComponent());
		}
	}
}

void ACoreHUD::ReceivedPlayerClassChange(ACorePlayerState* PlayerState, UPlayerClassComponent* PlayerClass)
{
	OnReceivedPlayerClass(PlayerState ? PlayerState->GetPlayerClassComponent() : nullptr);
}

void ACoreHUD::ReceivedPlayerPrompt(const FPromptHandle& PromptHandle)
{
	if (!GetOwningCorePlayerController())
	{
		return;
	}

	const FPromptData& PromptData = GetOwningCorePlayerController()->GetPlayerPromptComponent()->GetPromptData(PromptHandle);

	if (!PromptData.IsValid())
	{
		return;
	}

	OnReceivedPlayerPrompt(PromptHandle);
}